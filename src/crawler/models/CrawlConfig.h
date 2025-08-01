#pragma once

#include <string>
#include <chrono>
#include <set>
#include <curl/curl.h>

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
    
    // Timeout for HTTP requests (in milliseconds) - increased for SPA rendering
    std::chrono::milliseconds requestTimeout{30000};
    
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
    
    // Whether to enable SPA rendering with headless browser
    bool spaRenderingEnabled = true;
    
    // Browserless service URL for SPA rendering
    std::string browserlessUrl = "http://browserless:3000";
    
    // Whether to include full content in results (similar to SPA render API)
    bool includeFullContent = false;
    
    // === RETRY CONFIGURATION ===
    
    // Maximum number of retry attempts per URL
    int maxRetries = 3;
    
    // Base delay for first retry (in milliseconds)
    std::chrono::milliseconds baseRetryDelay{1000};
    
    // Exponential backoff multiplier
    float backoffMultiplier = 2.0f;
    
    // Maximum retry delay (in milliseconds) 
    std::chrono::milliseconds maxRetryDelay{30000};
    
    // HTTP status codes that should trigger a retry
    std::set<int> retryableHttpCodes = {
        408, // Request Timeout
        429, // Too Many Requests
        500, // Internal Server Error
        502, // Bad Gateway
        503, // Service Unavailable
        504, // Gateway Timeout
        520, // Cloudflare Unknown Error
        521, // Cloudflare Web Server Is Down
        522, // Cloudflare Connection Timed Out
        523, // Cloudflare Origin Is Unreachable
        524  // Cloudflare A Timeout Occurred
    };
    
    // CURL error codes that should trigger a retry
    std::set<CURLcode> retryableCurlCodes = {
        CURLE_OPERATION_TIMEDOUT,    // Timeout reached
        CURLE_COULDNT_CONNECT,       // Couldn't connect to server
        CURLE_COULDNT_RESOLVE_HOST,  // Couldn't resolve host
        CURLE_RECV_ERROR,            // Failure receiving network data
        CURLE_SEND_ERROR,            // Failure sending network data
        CURLE_GOT_NOTHING,           // Nothing was returned from the server
        CURLE_PARTIAL_FILE,          // File transfer shorter or larger than expected
        CURLE_SSL_CONNECT_ERROR,     // SSL connection error
        CURLE_PEER_FAILED_VERIFICATION // SSL peer certificate verification failed
    };
    
    // === CIRCUIT BREAKER CONFIGURATION ===
    
    // Number of consecutive failures before opening circuit breaker
    int circuitBreakerFailureThreshold = 5;
    
    // Time to wait before trying to close circuit breaker (in minutes)
    std::chrono::minutes circuitBreakerResetTime{5};
    
    // === DOMAIN RATE LIMITING ===
    
    // Additional delay for rate-limited responses (429) in seconds
    std::chrono::seconds rateLimitDelay{60};
}; 