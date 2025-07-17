#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>

struct CrawlResult {
    // The URL that was crawled
    std::string url;
    
    // The final URL after redirects (if any)
    std::string finalUrl;
    
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
    
    // Timestamp when the page was queued
    std::chrono::system_clock::time_point queuedAt;
    // Timestamp when the page started downloading
    std::chrono::system_clock::time_point startedAt;
    // Timestamp when the page finished downloading
    std::chrono::system_clock::time_point finishedAt;
    
    // Status of the crawl (queued, downloading, downloaded, failed)
    std::string crawlStatus;
    
    // Domain for this crawl result
    std::string domain;
    
    // Timestamp when the page was crawled (legacy, for compatibility)
    std::chrono::system_clock::time_point crawlTime;
    
    // Size of the response in bytes
    size_t contentSize;
    
    // Whether the crawl was successful
    bool success;
    
    // Error message if the crawl failed
    std::optional<std::string> errorMessage;
}; 