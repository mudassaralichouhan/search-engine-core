#pragma once

#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <curl/curl.h>

struct PageFetchResult {
    bool success;
    int statusCode;
    std::string contentType;
    std::string content;
    std::string errorMessage;
    std::string finalUrl;  // After redirects
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
    
    // Set a callback for progress updates
    void setProgressCallback(std::function<void(size_t, size_t)> callback);
    
    // Set custom headers
    void setCustomHeaders(const std::vector<std::pair<std::string, std::string>>& headers);
    
    // Set proxy
    void setProxy(const std::string& proxy);
    
    // Enable/disable SSL verification
    void setVerifySSL(bool verify);

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
    std::function<void(size_t, size_t)> progressCallback;
    std::vector<std::pair<std::string, std::string>> customHeaders;
    std::string proxy;
    bool verifySSL;
}; 