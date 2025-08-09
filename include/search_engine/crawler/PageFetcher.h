#pragma once

#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <curl/curl.h>
#include "BrowserlessClient.h"

struct PageFetchResult {
    bool success;
    int statusCode;
    std::string contentType;
    std::string content;
    std::string errorMessage;
    std::string finalUrl;
    CURLcode curlCode = CURLE_OK;
};

class PageFetcher {
public:
    PageFetcher(const std::string& userAgent,
                std::chrono::milliseconds timeout,
                bool followRedirects = true,
                size_t maxRedirects = 5);
    ~PageFetcher();

    PageFetchResult fetch(const std::string& url);
    PageFetchResult fetchWithDomainRestriction(const std::string& url, const std::string& seedDomain, std::function<std::string(const std::string&)> extractDomainFn, size_t maxRedirects = 5);

    void setProgressCallback(std::function<void(size_t, size_t)> callback);
    void setCustomHeaders(const std::vector<std::pair<std::string, std::string>>& headers);
    void setProxy(const std::string& proxy);
    void setVerifySSL(bool verify);
    void setSpaRendering(bool enable, const std::string& browserless_url = "http://browserless:3000");
    bool isSpaPage(const std::string& html, const std::string& url);

private:
    void initCurl();
    void cleanupCurl();
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
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
    std::string cookieJarPath;

    bool spaRenderingEnabled;
    std::unique_ptr<BrowserlessClient> browserlessClient;
};


