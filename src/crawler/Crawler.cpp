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

void Crawler::addSeedURL(const std::string& url, bool force) {
    LOG_INFO("Adding seed URL: " + url + (force ? " (force)" : ""));
    
    // Set seed domain if this is the first URL and domain restriction is enabled
    if (config.restrictToSeedDomain && seedDomain.empty()) {
        seedDomain = urlFrontier->extractDomain(url);
        LOG_INFO("Set seed domain to: " + seedDomain);
    }
    
    urlFrontier->addURL(url, force);
    // Add a CrawlResult for this URL with status 'queued'
    CrawlResult result;
    result.url = url;
    result.domain = urlFrontier->extractDomain(url);
    result.crawlStatus = "queued";
    result.queuedAt = std::chrono::system_clock::now();
    {
        std::lock_guard<std::mutex> lock(resultsMutex);
        results.push_back(result);
    }
}

std::vector<CrawlResult> Crawler::getResults() {
    std::lock_guard<std::mutex> lock(resultsMutex);
    LOG_DEBUG("Getting results, count: " + std::to_string(results.size()));
    return results;
}

void Crawler::crawlLoop() {
    LOG_DEBUG("Entering crawl loop");
    while (isRunning) {
        std::string url = urlFrontier->getNextURL();
        if (url.empty()) {
            LOG_INFO("No more URLs to crawl, exiting crawl loop");
            isRunning = false;
            break;
        }
        
        if (urlFrontier->isVisited(url)) {
            LOG_DEBUG("URL already visited, skipping: " + url);
            continue;
        }
        
        // Set status to 'downloading' and startedAt
        {
            std::lock_guard<std::mutex> lock(resultsMutex);
            for (auto& r : results) {
                if (r.url == url && r.crawlStatus == "queued") {
                    r.crawlStatus = "downloading";
                    r.startedAt = std::chrono::system_clock::now();
                }
            }
        }
        
        CrawlResult result = processURL(url);
        result.finishedAt = std::chrono::system_clock::now();
        result.domain = urlFrontier->extractDomain(url);
        
        // Set crawlStatus based on result.success
        if (result.success) {
            result.crawlStatus = "downloaded";
        } else {
            result.crawlStatus = "failed";
        }
        
        // Store the result (replace the old one for this URL)
        {
            std::lock_guard<std::mutex> lock(resultsMutex);
            auto it = std::find_if(results.begin(), results.end(), [&](const CrawlResult& r) { return r.url == url; });
            if (it != results.end()) {
                *it = result;
            } else {
                results.push_back(result);
            }
            LOG_INFO("Added result for URL: " + url + ", total results: " + std::to_string(results.size()));
        }
        
        if (storage) {
            auto storeResult = storage->storeCrawlResult(result);
            if (storeResult.success) {
                LOG_INFO("Successfully saved crawl result to database for URL: " + url);
            } else {
                LOG_ERROR("Failed to save crawl result to database for URL: " + url + " - " + storeResult.message);
            }
            // Store crawl log for history
            search_engine::storage::CrawlLog log;
            log.url = result.url;
            log.domain = urlFrontier->extractDomain(result.url);
            log.crawlTime = result.crawlTime;
            log.status = result.success ? "SUCCESS" : "FAILED";
            log.httpStatusCode = result.statusCode;
            log.errorMessage = result.errorMessage;
            log.contentSize = result.contentSize;
            log.contentType = result.contentType;
            log.links = result.links;
            log.title = result.title;
            log.description = result.metaDescription;
            auto logResult = storage->storeCrawlLog(log);
            if (logResult.success) {
                LOG_INFO("Crawl log entry saved for URL: " + url);
            } else {
                LOG_ERROR("Failed to save crawl log for URL: " + url + " - " + logResult.message);
            }
        } else {
            LOG_WARNING("No storage configured, crawl result not saved to database for URL: " + url);
        }
        
        urlFrontier->markVisited(url);
        
        if (results.size() >= config.maxPages) {
            LOG_INFO("Reached maximum pages limit (" + std::to_string(config.maxPages) + "), stopping crawler");
            isRunning = false;
            break;
        }
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
    
    // Always store the result data, regardless of status code
    result.statusCode = fetchResult.statusCode;
    result.contentType = fetchResult.contentType;
    result.contentSize = fetchResult.content.size();
    result.finalUrl = fetchResult.finalUrl;  // Store the final URL after redirects
    
    // Always store raw content if configured, regardless of status
    if (config.storeRawContent) {
        result.rawContent = fetchResult.content;
    }
    
    LOG_INFO("=== HTTP STATUS: " + std::to_string(fetchResult.statusCode) + " === for URL: " + url);
    
    // Log the final URL if it's different from the original
    if (!fetchResult.finalUrl.empty() && fetchResult.finalUrl != url) {
        LOG_INFO("Final URL after redirects: " + fetchResult.finalUrl);
    }
    
    // Check if the fetch was successful (2xx status codes)
    if (fetchResult.statusCode >= 200 && fetchResult.statusCode < 300) {
        result.success = true;
        LOG_INFO("Page fetched successfully: " + url + " Status: " + std::to_string(fetchResult.statusCode));
    } else {
        result.success = false;
        if (fetchResult.statusCode >= 300 && fetchResult.statusCode < 400) {
            result.errorMessage = "HTTP Redirect: " + std::to_string(fetchResult.statusCode);
            LOG_INFO("HTTP REDIRECT: Status " + std::to_string(fetchResult.statusCode) + " for URL: " + url);
        } else if (fetchResult.statusCode >= 400) {
            result.errorMessage = "HTTP Error: " + std::to_string(fetchResult.statusCode);
            LOG_WARNING("HTTP ERROR: Status " + std::to_string(fetchResult.statusCode) + " for URL: " + url);
        } else if (!fetchResult.errorMessage.empty()) {
            result.errorMessage = fetchResult.errorMessage;
            LOG_ERROR("Failed to fetch page: " + url + " Error: " + fetchResult.errorMessage);
        }
    }
    
    // Parse the content if it's HTML, regardless of status code
    if (fetchResult.contentType.find("text/html") != std::string::npos && !fetchResult.content.empty()) {
        LOG_DEBUG("Content is HTML, parsing...");
        auto parsedContent = contentParser->parse(fetchResult.content, url);
        
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
    
    int addedCount = 0;
    int skippedCount = 0;
    
    for (const auto& link : links) {
        // Check if URL is allowed by robots.txt
        if (!config.respectRobotsTxt || robotsParser->isAllowed(link, config.userAgent)) {
            // Check domain restriction if enabled
            if (config.restrictToSeedDomain && !isSameDomain(link)) {
                LOG_DEBUG("Skipping URL from different domain: " + link);
                skippedCount++;
                continue;
            }
            
            urlFrontier->addURL(link);
            addedCount++;
        }
    }
    
    LOG_INFO("Added " + std::to_string(addedCount) + " URLs to frontier, skipped " + 
             std::to_string(skippedCount) + " URLs (domain restriction: " + 
             (config.restrictToSeedDomain ? "enabled" : "disabled") + ")");
}

bool Crawler::isSameDomain(const std::string& url) const {
    if (seedDomain.empty()) {
        // If no seed domain is set, allow all URLs
        return true;
    }
    
    std::string urlDomain = urlFrontier->extractDomain(url);
    bool isSame = (urlDomain == seedDomain);
    
    LOG_DEBUG("Domain check - URL: " + url + ", URL domain: " + urlDomain + 
              ", Seed domain: " + seedDomain + ", Same domain: " + (isSame ? "yes" : "no"));
    
    return isSame;
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
             ", maxDepth: " + std::to_string(config.maxDepth) + 
             ", restrictToSeedDomain: " + (config.restrictToSeedDomain ? "true" : "false") + 
             ", followRedirects: " + (config.followRedirects ? "true" : "false") + 
             ", maxRedirects: " + std::to_string(config.maxRedirects));
    
    // Update PageFetcher configuration
    updatePageFetcherConfig();
}

void Crawler::updatePageFetcherConfig() {
    if (pageFetcher) {
        // Recreate PageFetcher with new configuration
        pageFetcher = std::make_unique<PageFetcher>(
            config.userAgent,
            config.requestTimeout,
            config.followRedirects,
            config.maxRedirects
        );
        LOG_INFO("Updated PageFetcher configuration");
    }
}

CrawlConfig Crawler::getConfig() const {
    std::lock_guard<std::mutex> lock(resultsMutex);
    return config;
} 