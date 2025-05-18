#include "PageFetcher.h"
#include <sstream>
#include <stdexcept>

PageFetcher::PageFetcher(const std::string& userAgent,
                         std::chrono::milliseconds timeout,
                         bool followRedirects,
                         size_t maxRedirects)
    : userAgent(userAgent)
    , timeout(timeout)
    , followRedirects(followRedirects)
    , maxRedirects(maxRedirects)
    , verifySSL(true) {
    initCurl();
}

PageFetcher::~PageFetcher() {
    cleanupCurl();
}

void PageFetcher::initCurl() {
    curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

void PageFetcher::cleanupCurl() {
    if (curl) {
        curl_easy_cleanup(curl);
        curl = nullptr;
    }
}

PageFetchResult PageFetcher::fetch(const std::string& url) {
    PageFetchResult result;
    result.success = false;
    
    if (!curl) {
        result.errorMessage = "CURL not initialized";
        return result;
    }
    
    // Reset CURL options
    curl_easy_reset(curl);
    
    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    
    // Set user agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());
    
    // Set timeout
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout.count());
    
    // Set redirect options
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, followRedirects ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, static_cast<long>(maxRedirects));
    
    // Set SSL verification
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, verifySSL ? 1L : 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, verifySSL ? 2L : 0L);
    
    // Set proxy if configured
    if (!proxy.empty()) {
        curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
    }
    
    // Set custom headers
    struct curl_slist* headers = nullptr;
    for (const auto& header : customHeaders) {
        std::string headerStr = header.first + ": " + header.second;
        headers = curl_slist_append(headers, headerStr.c_str());
    }
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    
    // Set write callback
    std::string responseData;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
    
    // Set progress callback if configured
    if (userProgressCallback) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progressCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &userProgressCallback);
    }
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    // Clean up headers
    if (headers) {
        curl_slist_free_all(headers);
    }
    
    if (res != CURLE_OK) {
        result.errorMessage = curl_easy_strerror(res);
        return result;
    }
    
    // Get status code
    long statusCode;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
    result.statusCode = static_cast<int>(statusCode);
    
    // Get content type
    char* contentType = nullptr;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contentType);
    if (contentType) {
        result.contentType = contentType;
    }
    
    // Get final URL (after redirects)
    char* finalUrl = nullptr;
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &finalUrl);
    if (finalUrl) {
        result.finalUrl = finalUrl;
    }
    
    result.content = std::move(responseData);
    result.success = true;
    return result;
}

void PageFetcher::setProgressCallback(std::function<void(size_t, size_t)> callback) {
    userProgressCallback = std::move(callback);
}

void PageFetcher::setCustomHeaders(const std::vector<std::pair<std::string, std::string>>& headers) {
    customHeaders = headers;
}

void PageFetcher::setProxy(const std::string& proxyUrl) {
    proxy = proxyUrl;
}

void PageFetcher::setVerifySSL(bool verify) {
    verifySSL = verify;
}

size_t PageFetcher::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::string* responseData = static_cast<std::string*>(userp);
    size_t totalSize = size * nmemb;
    responseData->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

int PageFetcher::progressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    auto* callback = static_cast<std::function<void(size_t, size_t)>*>(clientp);
    if (callback && *callback) {
        (*callback)(static_cast<size_t>(dlnow), static_cast<size_t>(dltotal));
    }
    return 0;
} 