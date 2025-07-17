#pragma once

#include <string>
#include <chrono>

struct CrawlConfig {
    // Maximum number of pages to crawl
    size_t maxPages = 1000;
    
    // Maximum depth to crawl
    size_t maxDepth = 5;
    
    // Delay between requests to the same domain (in milliseconds)
    std::chrono::milliseconds politenessDelay{1000};
    
    // User agent string to use in requests
    std::string userAgent = "Hatefbot/1.0";
    
    // Maximum number of concurrent connections
    size_t maxConcurrentConnections = 10;
    
    // Timeout for HTTP requests (in milliseconds)
    std::chrono::milliseconds requestTimeout{5000};
    
    // Whether to respect robots.txt
    bool respectRobotsTxt = true;
    
    // Whether to follow redirects
    bool followRedirects = true;
    
    // Maximum number of redirects to follow
    size_t maxRedirects = 5;
    
    // Whether to store raw HTML content
    bool storeRawContent = true;
    
    // Whether to extract and store text content
    bool extractTextContent = true;
    
    // Whether to restrict crawling to the same domain as seed URL
    bool restrictToSeedDomain = true;
}; 