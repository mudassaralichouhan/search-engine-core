#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>

struct CrawlResult {
    // The URL that was crawled
    std::string url;
    
    // HTTP status code of the response
    int statusCode;
    
    // Content type of the response
    std::string contentType;
    
    // Raw HTML content (if storeRawContent is true)
    std::optional<std::string> rawContent;
    
    // Extracted text content (if extractTextContent is true)
    std::optional<std::string> textContent;
    
    // Title of the page
    std::optional<std::string> title;
    
    // Meta description
    std::optional<std::string> metaDescription;
    
    // List of links found on the page
    std::vector<std::string> links;
    
    // Timestamp when the page was crawled
    std::chrono::system_clock::time_point crawlTime;
    
    // Size of the response in bytes
    size_t contentSize;
    
    // Whether the crawl was successful
    bool success;
    
    // Error message if the crawl failed
    std::optional<std::string> errorMessage;
}; 