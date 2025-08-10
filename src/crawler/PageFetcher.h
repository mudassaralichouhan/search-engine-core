#pragma once

#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <curl/curl.h>
#include "BrowserlessClient.h"
#include "../../include/search_engine/crawler/BrowserlessClient.h"

struct PageFetchResult {
    bool success;
    int statusCode;
    std::string contentType;
    std::string content;
    std::string errorMessage;
    std::string finalUrl;  // After redirects
    CURLcode curlCode = CURLE_OK;  // CURL error code for better retry classification
};

class PageFetcher {
public:
    PageFetcher(const std::string& userAgent, 
                std::chrono::milliseconds timeout,
                bool followRedirects = true,
                size_t maxRedirects = 5);
    ~PageFetcher();

    // Fetch a page from a URL
    PageFetchResult fetch(const std::string& url);
    
    // Fetch a page, following redirects only if the new URL is on the same domain as the seed domain
    PageFetchResult fetchWithDomainRestriction(const std::string& url, const std::string& seedDomain, std::function<std::string(const std::string&)> extractDomainFn, size_t maxRedirects = 5);
    
    // Set a callback for progress updates
    void setProgressCallback(std::function<void(size_t, size_t)> callback);
    
    // Set custom headers
    void setCustomHeaders(const std::vector<std::pair<std::string, std::string>>& headers);
    
    // Set proxy
    void setProxy(const std::string& proxy);
    
    // Enable/disable SSL verification
    void setVerifySSL(bool verify);
    
    // Enable/disable SPA detection and rendering
    void setSpaRendering(bool enable,
                         const std::string& browserless_url = "http://browserless:3000",
                         bool useWebsocket = true,
                         size_t wsConnectionsPerCpu = 1);
    
    // Check if a page is likely a SPA (Single Page Application)
    bool isSpaPage(const std::string& html, const std::string& url);

private:
    // Initialize CURL
    void initCurl();
    
    // Clean up CURL
    void cleanupCurl();
    
    // Write callback for CURL
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
    
    // Progress callback for CURL
    static int progressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);

    CURL* curl;
    std::string userAgent;
    std::chrono::milliseconds timeout;
    bool followRedirects;
    size_t maxRedirects;
    std::function<void(size_t, size_t)> userProgressCallback;
    std::vector<std::pair<std::string, std::string>> customHeaders;
    std::string proxy;
    bool verifySSL;
    std::string cookieJarPath;  // Path to cookie jar file
    
    // SPA rendering support
    bool spaRenderingEnabled;
    std::unique_ptr<BrowserlessClient> browserlessClient;
    std::unique_ptr<search_engine::crawler::BrowserlessTransportClient> browserlessTransport;
}; 