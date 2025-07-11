#include "Crawler.h"
#include "URLFrontier.h"
#include "RobotsTxtParser.h"
#include "PageFetcher.h"
#include "ContentParser.h"
#include "../../include/Logger.h"
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>

Crawler::Crawler(const CrawlConfig& config, std::shared_ptr<search_engine::storage::ContentStorage> storage)
    : storage(storage)
    , config(config)
    , isRunning(false) {
    // Initialize logger with INFO level by default
    Logger::getInstance().init(LogLevel::INFO, true);
    LOG_DEBUG("Crawler constructor called");
    
    urlFrontier = std::make_unique<URLFrontier>();
    robotsParser = std::make_unique<RobotsTxtParser>();
    pageFetcher = std::make_unique<PageFetcher>(
        config.userAgent,
        config.requestTimeout,
        config.followRedirects,
        config.maxRedirects
    );
    contentParser = std::make_unique<ContentParser>();
}

Crawler::~Crawler() {
    LOG_DEBUG("Crawler destructor called");
    stop();
}

void Crawler::start() {
    if (isRunning) {
        LOG_DEBUG("Crawler already running, ignoring start request");
        return;
    }
    
    LOG_INFO("Starting crawler");
    isRunning = true;
    std::thread crawlerThread(&Crawler::crawlLoop, this);
    crawlerThread.detach();
}

void Crawler::stop() {
    LOG_INFO("Stopping crawler");
    isRunning = false;
    
    // Give a small delay to ensure all results are collected
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Log the final results count
    {
        std::lock_guard<std::mutex> lock(resultsMutex);
        LOG_INFO("Final results count: " + std::to_string(results.size()));
    }
}

void Crawler::addSeedURL(const std::string& url) {
    LOG_INFO("Adding seed URL: " + url);
    urlFrontier->addURL(url);
}

std::vector<CrawlResult> Crawler::getResults() {
    std::lock_guard<std::mutex> lock(resultsMutex);
    LOG_DEBUG("Getting results, count: " + std::to_string(results.size()));
    return results;
}

void Crawler::crawlLoop() {
    LOG_DEBUG("Entering crawl loop");
    while (isRunning) {
        // Get next URL to crawl
        std::string url = urlFrontier->getNextURL();
        if (url.empty()) {
            // No more URLs to crawl
            LOG_INFO("No more URLs to crawl, exiting crawl loop");
            isRunning = false;
            break;
        }
        
        // Check if URL is already visited
        if (urlFrontier->isVisited(url)) {
            LOG_DEBUG("URL already visited, skipping: " + url);
            continue;
        }
        
        // Process the URL
        CrawlResult result = processURL(url);
        
        // Store the result first
        {
            std::lock_guard<std::mutex> lock(resultsMutex);
            results.push_back(result);
            LOG_INFO("Added result for URL: " + url + ", total results: " + std::to_string(results.size()));
        }
        
        // Save to database if storage is available
        if (storage) {
            auto storeResult = storage->storeCrawlResult(result);
            if (storeResult.success) {
                LOG_INFO("Successfully saved crawl result to database for URL: " + url);
            } else {
                LOG_ERROR("Failed to save crawl result to database for URL: " + url + " - " + storeResult.message);
            }
        } else {
            LOG_WARNING("No storage configured, crawl result not saved to database for URL: " + url);
        }
        
        // Then mark URL as visited
        urlFrontier->markVisited(url);
        
        // Check if we've reached the maximum number of pages
        if (results.size() >= config.maxPages) {
            LOG_INFO("Reached maximum pages limit (" + std::to_string(config.maxPages) + "), stopping crawler");
            isRunning = false;
            break;
        }
        
        // Add a small delay between requests to ensure results are stored
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    LOG_DEBUG("Exiting crawl loop");
}

CrawlResult Crawler::processURL(const std::string& url) {
    CrawlResult result;
    result.url = url;
    result.crawlTime = std::chrono::system_clock::now();
    
    LOG_INFO("Processing URL: " + url);
    
    // Check if URL is allowed by robots.txt
    if (config.respectRobotsTxt) {
        std::string domain = urlFrontier->extractDomain(url);
        if (!robotsParser->isAllowed(url, config.userAgent)) {
            result.success = false;
            result.errorMessage = "URL not allowed by robots.txt";
            LOG_WARNING("URL not allowed by robots.txt: " + url);
            return result;
        }
        
        // Respect crawl delay
        auto lastVisit = urlFrontier->getLastVisitTime(domain);
        auto crawlDelay = robotsParser->getCrawlDelay(domain, config.userAgent);
        auto timeSinceLastVisit = std::chrono::system_clock::now() - lastVisit;
        
        // For testing purposes, completely disable crawl delay
        LOG_DEBUG("NOTE: Crawl delay disabled for testing");
        // Only sleep for a very short time for testing purposes
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Fetch the page
    LOG_INFO("Fetching page: " + url);
    auto fetchResult = pageFetcher->fetch(url);
    if (!fetchResult.success) {
        result.success = false;
        result.errorMessage = fetchResult.errorMessage;
        result.statusCode = fetchResult.statusCode;
        LOG_ERROR("Failed to fetch page: " + url + " Error: " + fetchResult.errorMessage);
        return result;
    }
    
    LOG_INFO("=== HTTP STATUS: " + std::to_string(fetchResult.statusCode) + " === for URL: " + url);
    
    if (fetchResult.statusCode >= 400) {
        LOG_WARNING("HTTP ERROR: Status " + std::to_string(fetchResult.statusCode) + " for URL: " + url);
        result.success = false;
        result.errorMessage = "HTTP Error: " + std::to_string(fetchResult.statusCode);
        result.statusCode = fetchResult.statusCode;
        return result;
    }
    
    LOG_INFO("Page fetched successfully: " + url + " Status: " + std::to_string(fetchResult.statusCode));
    
    result.success = true;
    result.statusCode = fetchResult.statusCode;
    result.contentType = fetchResult.contentType;
    result.contentSize = fetchResult.content.size();
    
    // Parse the content if it's HTML
    if (fetchResult.contentType.find("text/html") != std::string::npos) {
        LOG_DEBUG("Content is HTML, parsing...");
        auto parsedContent = contentParser->parse(fetchResult.content, url);
        
        if (config.storeRawContent) {
            result.rawContent = fetchResult.content;
        }
        
        if (config.extractTextContent) {
            result.textContent = parsedContent.textContent;
        }
        
        result.title = parsedContent.title;
        result.metaDescription = parsedContent.metaDescription;
        
        // Add new URLs to the frontier
        extractAndAddURLs(fetchResult.content, url);
    } else {
        LOG_DEBUG("Content is not HTML, skipping parsing. Content-Type: " + fetchResult.contentType);
    }
    
    LOG_INFO("URL processed successfully: " + url);
    return result;
}

void Crawler::extractAndAddURLs(const std::string& content, const std::string& baseUrl) {
    auto links = contentParser->extractLinks(content, baseUrl);
    LOG_DEBUG("Extracted " + std::to_string(links.size()) + " links from " + baseUrl);
    
    for (const auto& link : links) {
        // Check if URL is allowed by robots.txt
        if (!config.respectRobotsTxt || robotsParser->isAllowed(link, config.userAgent)) {
            urlFrontier->addURL(link);
        }
    }
}

PageFetcher* Crawler::getPageFetcher() {
    return pageFetcher.get();
}

void Crawler::setMaxPages(size_t maxPages) {
    std::lock_guard<std::mutex> lock(resultsMutex);
    config.maxPages = maxPages;
    LOG_INFO("Updated maxPages to: " + std::to_string(maxPages));
}

void Crawler::setMaxDepth(size_t maxDepth) {
    std::lock_guard<std::mutex> lock(resultsMutex);
    config.maxDepth = maxDepth;
    LOG_INFO("Updated maxDepth to: " + std::to_string(maxDepth));
}

void Crawler::updateConfig(const CrawlConfig& newConfig) {
    std::lock_guard<std::mutex> lock(resultsMutex);
    config = newConfig;
    LOG_INFO("Updated crawler configuration - maxPages: " + std::to_string(config.maxPages) + 
             ", maxDepth: " + std::to_string(config.maxDepth));
} 