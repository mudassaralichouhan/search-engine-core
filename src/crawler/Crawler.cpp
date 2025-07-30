#include "Crawler.h"
#include "URLFrontier.h"
#include "RobotsTxtParser.h"
#include "PageFetcher.h"
#include "ContentParser.h"
#include "../../include/Logger.h"
#include "../../include/crawler/CrawlLogger.h"
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

Crawler::Crawler(const CrawlConfig& config, std::shared_ptr<search_engine::storage::ContentStorage> storage)
    : storage(storage)
    , config(config)
    , isRunning(false) {
    // Initialize logger with DEBUG level to troubleshoot textContent issue
    Logger::getInstance().init(LogLevel::DEBUG, true);
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
    CrawlLogger::broadcastLog("Starting crawler", "info");
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

void Crawler::reset() {
    LOG_INFO("Resetting crawler state");
    
    // Stop crawling if it's running
    if (isRunning) {
        stop();
    }
    
    // Clear all state
    {
        std::lock_guard<std::mutex> lock(resultsMutex);
        results.clear();
        seedDomain.clear();
    }
    
    // Reset URL frontier
    if (urlFrontier) {
        urlFrontier = std::make_unique<URLFrontier>();
    }
    
    LOG_INFO("Crawler state reset completed");
}

void Crawler::addSeedURL(const std::string& url, bool force) {
    LOG_INFO("Adding seed URL: " + url + (force ? " (force)" : ""));
    CrawlLogger::broadcastLog("Adding seed URL: " + url + (force ? " (force)" : ""), "info");
    
    // Set seed domain if this is the first URL and domain restriction is enabled
    if (config.restrictToSeedDomain && seedDomain.empty()) {
        seedDomain = urlFrontier->extractDomain(url);
        LOG_INFO("Set seed domain to: " + seedDomain);
        CrawlLogger::broadcastLog("Set seed domain to: " + seedDomain, "info");
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
            CrawlLogger::broadcastLog("No more URLs to crawl, exiting crawl loop", "info");
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
                    CrawlLogger::broadcastLog("Started downloading: " + url, "info");
                }
            }
        }
        
        CrawlResult result = processURL(url);
        result.finishedAt = std::chrono::system_clock::now();
        result.domain = urlFrontier->extractDomain(url);
        
        // Set crawlStatus based on result.success
        if (result.success) {
            result.crawlStatus = "downloaded";
            CrawlLogger::broadcastLog("Successfully downloaded: " + url + " (Status: " + std::to_string(result.statusCode) + ")", "info");
        } else {
            result.crawlStatus = "failed";
            CrawlLogger::broadcastLog("Failed to download: " + url + " - " + (result.errorMessage.has_value() ? result.errorMessage.value() : "Unknown error"), "error");
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
                CrawlLogger::broadcastLog("Saved to database: " + url, "info");
            } else {
                LOG_ERROR("Failed to save crawl result to database for URL: " + url + " - " + storeResult.message);
                CrawlLogger::broadcastLog("Database save failed for: " + url + " - " + storeResult.message, "error");
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
            CrawlLogger::broadcastLog("Reached maximum pages limit (" + std::to_string(config.maxPages) + "), stopping crawler", "warning");
            isRunning = false;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    LOG_DEBUG("Exiting crawl loop");
}

CrawlResult Crawler::processURL(const std::string& url) {
    LOG_INFO("🚀🚀🚀 BINARY UPDATE TEST - NEW VERSION LOADED 🚀🚀🚀");
    LOG_DEBUG("[processURL] Called with url: " + url);
    CrawlResult result;
    result.url = url;
    result.crawlTime = std::chrono::system_clock::now();
    LOG_DEBUG("[processURL] Initialized CrawlResult");
    
    LOG_INFO("Processing URL: " + url);
    
    // Check if URL is allowed by robots.txt
    if (config.respectRobotsTxt) {
        std::string domain = urlFrontier->extractDomain(url);
        LOG_DEBUG("[processURL] Extracted domain: " + domain);
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
        LOG_DEBUG("[processURL] lastVisit: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(lastVisit.time_since_epoch()).count()) + ", crawlDelay: " + std::to_string(crawlDelay.count()) + ", timeSinceLastVisit: " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(timeSinceLastVisit).count()));
        // For testing purposes, completely disable crawl delay
        LOG_DEBUG("NOTE: Crawl delay disabled for testing");
        // Only sleep for a very short time for testing purposes
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Fetch the page
    LOG_INFO("Fetching page: " + url);
    auto fetchResult = pageFetcher->fetch(url);
    LOG_DEBUG("[processURL] fetchResult.statusCode: " + std::to_string(fetchResult.statusCode));
    LOG_DEBUG("[processURL] fetchResult.contentType: " + fetchResult.contentType);
    LOG_DEBUG("[processURL] fetchResult.content (first 200): " + (fetchResult.content.size() > 200 ? fetchResult.content.substr(0, 200) + "..." : fetchResult.content));

    // If SPA rendering is enabled and the page is detected as SPA, fetch again with headless browser
    if (pageFetcher->isSpaPage(fetchResult.content, url)) {
        LOG_INFO("SPA detected for URL: " + url + ". Fetching with headless browser...");
        // Ensure SPA rendering is enabled (should be, but double-check)
        pageFetcher->setSpaRendering(true, "http://browserless:3000");
        auto spaFetchResult = pageFetcher->fetch(url);
        LOG_DEBUG("[processURL] spaFetchResult.statusCode: " + std::to_string(spaFetchResult.statusCode));
        LOG_DEBUG("[processURL] spaFetchResult.contentType: " + spaFetchResult.contentType);
        LOG_DEBUG("[processURL] spaFetchResult.content (first 200): " + (spaFetchResult.content.size() > 200 ? spaFetchResult.content.substr(0, 200) + "..." : spaFetchResult.content));
        if (spaFetchResult.success && !spaFetchResult.content.empty()) {
            LOG_INFO("Successfully fetched SPA-rendered HTML for URL: " + url);
            fetchResult = spaFetchResult;
        } else {
            LOG_WARNING("Failed to fetch SPA-rendered HTML for URL: " + url + ". Using original content.");
        }
    }
    
    // Always store the result data, regardless of status code
    result.statusCode = fetchResult.statusCode;
    result.contentType = fetchResult.contentType;
    result.contentSize = fetchResult.content.size();
    result.finalUrl = fetchResult.finalUrl;  // Store the final URL after redirects
    LOG_DEBUG("[processURL] Stored result status, contentType, contentSize, finalUrl");
    
    // Store raw content based on includeFullContent setting (similar to SPA render API)
    if (config.storeRawContent) {
        if (config.includeFullContent) {
            // Store full content when includeFullContent is enabled
            result.rawContent = fetchResult.content;
            LOG_DEBUG("[processURL] Stored full rawContent (includeFullContent=true)");
        } else {
            // Store only a preview when includeFullContent is disabled (like SPA render API)
            std::string preview = fetchResult.content.substr(0, 500);
            if (fetchResult.content.size() > 500) preview += "...";
            result.rawContent = preview;
            LOG_DEBUG("[processURL] Stored rawContent preview (includeFullContent=false)");
        }
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
        LOG_INFO("🔍 TEXTCONTENT DEBUG: Content is HTML, parsing... Content-Type: " + fetchResult.contentType);
        auto parsedContent = contentParser->parse(fetchResult.content, url);
        LOG_INFO("🔍 TEXTCONTENT DEBUG: Parsed title: " + parsedContent.title);
        LOG_INFO("🔍 TEXTCONTENT DEBUG: Parsed textContent length: " + std::to_string(parsedContent.textContent.size()));
        LOG_INFO("🔍 TEXTCONTENT DEBUG: extractTextContent config: " + std::string(config.extractTextContent ? "true" : "false"));
        if (config.extractTextContent) {
            result.textContent = parsedContent.textContent;
            LOG_INFO("🔍 TEXTCONTENT DEBUG: ✅ STORED textContent with length: " + std::to_string(result.textContent ? result.textContent->size() : 0));
        } else {
            LOG_INFO("🔍 TEXTCONTENT DEBUG: ❌ NOT storing textContent - config disabled");
        }
        result.title = parsedContent.title;
        result.metaDescription = parsedContent.metaDescription;
        // Add new URLs to the frontier
        extractAndAddURLs(fetchResult.content, url);
    } else {
        LOG_INFO("🔍 TEXTCONTENT DEBUG: ❌ Content is NOT HTML, skipping parsing. Content-Type: " + fetchResult.contentType);
    }
    
    // CRITICAL DEBUG: Log the contentType to see why HTML parsing is skipped
    LOG_INFO("🔍 CONTENTTYPE DEBUG: fetchResult.contentType = '" + fetchResult.contentType + "'");
    LOG_INFO("🔍 CONTENTTYPE DEBUG: content.empty() = " + std::string(fetchResult.content.empty() ? "true" : "false"));
    LOG_INFO("🔍 CONTENTTYPE DEBUG: content.size() = " + std::to_string(fetchResult.content.size()));
    
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
    
    // Enhanced domain matching - handle www prefix and subdomain variations
    auto normalizeForComparison = [](const std::string& domain) -> std::string {
        std::string normalized = domain;
        // Convert to lowercase
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
        // Remove www. prefix for comparison
        if (normalized.substr(0, 4) == "www.") {
            normalized = normalized.substr(4);
        }
        return normalized;
    };
    
    std::string normalizedUrlDomain = normalizeForComparison(urlDomain);
    std::string normalizedSeedDomain = normalizeForComparison(seedDomain);
    
    bool isSame = (normalizedUrlDomain == normalizedSeedDomain);
    
    LOG_DEBUG("Domain check - URL: " + url + ", URL domain: " + urlDomain + 
              " (normalized: " + normalizedUrlDomain + "), Seed domain: " + seedDomain + 
              " (normalized: " + normalizedSeedDomain + "), Same domain: " + (isSame ? "yes" : "no"));
    
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
        
        // Preserve SPA rendering settings from config
        if (config.spaRenderingEnabled) {
            pageFetcher->setSpaRendering(true, config.browserlessUrl);
            LOG_INFO("Enabled SPA rendering with browserless URL: " + config.browserlessUrl);
        }
        
        // Preserve SSL verification settings (typically disabled for problematic sites)
        pageFetcher->setVerifySSL(false);
        
        LOG_INFO("Updated PageFetcher configuration");
    }
}

CrawlConfig Crawler::getConfig() const {
    std::lock_guard<std::mutex> lock(resultsMutex);
    return config;
} 