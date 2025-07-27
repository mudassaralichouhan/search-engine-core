#pragma once

#include <string>
#include <memory>
#include <vector>
#include <queue>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include "models/CrawlResult.h"
#include "models/CrawlConfig.h"
#include "../../include/search_engine/storage/ContentStorage.h"

class URLFrontier;
class RobotsTxtParser;
class PageFetcher;
class ContentParser;

class Crawler {
public:
    Crawler(const CrawlConfig& config, std::shared_ptr<search_engine::storage::ContentStorage> storage = nullptr);
    ~Crawler();

    // Start the crawling process
    void start();
    
    // Stop the crawling process
    void stop();
    
    // Reset crawler state (clear results, visited URLs, and seed domain)
    void reset();
    
    // Add a seed URL to start crawling from
    void addSeedURL(const std::string& url, bool force = false);
    
    // Get the current crawl results
    std::vector<CrawlResult> getResults();
    const std::vector<CrawlResult>& getResults() const { return results; }
    std::vector<CrawlResult> getResultsCopy() const {
        std::lock_guard<std::mutex> lock(resultsMutex);
        return results;
    }
    
    // Get access to the PageFetcher for configuration
    PageFetcher* getPageFetcher();
    
    // Update crawler configuration
    void setMaxPages(size_t maxPages);
    void setMaxDepth(size_t maxDepth);
    void updateConfig(const CrawlConfig& newConfig);

    // Get current crawler configuration
    CrawlConfig getConfig() const;

    std::shared_ptr<search_engine::storage::ContentStorage> getStorage() const { return storage; }

    // Process a single URL
    CrawlResult processURL(const std::string& url);
    
private:
    // Main crawling loop
    void crawlLoop();
    
    // Extract and add new URLs from the page content
    void extractAndAddURLs(const std::string& content, const std::string& baseURL);
    
    // Check if URL belongs to the seed domain
    bool isSameDomain(const std::string& url) const;
    
    // Update PageFetcher configuration
    void updatePageFetcherConfig();

    std::unique_ptr<URLFrontier> urlFrontier;
    std::unique_ptr<RobotsTxtParser> robotsParser;
    std::unique_ptr<PageFetcher> pageFetcher;
    std::unique_ptr<ContentParser> contentParser;
    std::shared_ptr<search_engine::storage::ContentStorage> storage;
    
    CrawlConfig config;
    std::atomic<bool> isRunning;
    mutable std::mutex resultsMutex;
    std::vector<CrawlResult> results;
    std::unordered_set<std::string> visitedURLs;
    std::string seedDomain;  // Domain of the first seed URL
}; 