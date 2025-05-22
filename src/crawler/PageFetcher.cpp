#include "PageFetcher.h"
#include "../../include/Logger.h"
#include <sstream>
#include <stdexcept>
#include <algorithm>

PageFetcher::PageFetcher(const std::string& userAgent,
                         std::chrono::milliseconds timeout,
                         bool followRedirects,
                         size_t maxRedirects)
    : userAgent(userAgent)
    , timeout(timeout)
    , followRedirects(followRedirects)
    , maxRedirects(maxRedirects)
    , verifySSL(true) {
    LOG_DEBUG("PageFetcher constructor called with userAgent: " + userAgent);
    initCurl();
}

PageFetcher::~PageFetcher() {
    LOG_DEBUG("PageFetcher destructor called");
    cleanupCurl();
}

void PageFetcher::initCurl() {
    LOG_DEBUG("PageFetcher::initCurl called");
    curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize CURL");
        throw std::runtime_error("Failed to initialize CURL");
    }
    LOG_DEBUG("CURL initialized successfully");
}

void PageFetcher::cleanupCurl() {
    LOG_DEBUG("PageFetcher::cleanupCurl called");
    if (curl) {
        curl_easy_cleanup(curl);
        curl = nullptr;
        LOG_DEBUG("CURL cleaned up");
    }
}

PageFetchResult PageFetcher::fetch(const std::string& url) {
    LOG_INFO("PageFetcher::fetch called for URL: " + url);
    PageFetchResult result;
    result.success = false;
    
    if (!curl) {
        result.errorMessage = "CURL not initialized";
        LOG_ERROR("Error: " + result.errorMessage);
        // Attempt to re-initialize curl
        try {
            initCurl();
            LOG_DEBUG("Re-initialized CURL");
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to re-initialize CURL: " + std::string(e.what()));
            return result;
        }
    }
    
    // Use a local curl handle for this request to avoid multi-threading issues
    CURL* localCurl = curl_easy_init();
    if (!localCurl) {
        result.errorMessage = "Failed to create local CURL handle";
        LOG_ERROR("Error: " + result.errorMessage);
        return result;
    }
    
    LOG_DEBUG("Created local CURL handle");
    
    // Reset CURL options
    LOG_TRACE("Setting CURL options");
    
    // Set URL
    LOG_TRACE("Setting URL: " + url);
    curl_easy_setopt(localCurl, CURLOPT_URL, url.c_str());
    
    // Set user agent
    LOG_TRACE("Setting user agent: " + userAgent);
    curl_easy_setopt(localCurl, CURLOPT_USERAGENT, userAgent.c_str());
    
    // Set timeout
    long timeoutMs = static_cast<long>(timeout.count());
    LOG_DEBUG("Setting timeout: " + std::to_string(timeoutMs) + "ms");
    curl_easy_setopt(localCurl, CURLOPT_TIMEOUT_MS, timeoutMs);
    
    // Set redirect options
    LOG_TRACE("Setting follow location: " + std::string(followRedirects ? "true" : "false"));
    curl_easy_setopt(localCurl, CURLOPT_FOLLOWLOCATION, followRedirects ? 1L : 0L);
    LOG_TRACE("Setting max redirects: " + std::to_string(maxRedirects));
    curl_easy_setopt(localCurl, CURLOPT_MAXREDIRS, static_cast<long>(maxRedirects));
    
    // Set SSL verification
    LOG_TRACE("Setting SSL verification: " + std::string(verifySSL ? "true" : "false"));
    curl_easy_setopt(localCurl, CURLOPT_SSL_VERIFYPEER, verifySSL ? 1L : 0L);
    curl_easy_setopt(localCurl, CURLOPT_SSL_VERIFYHOST, verifySSL ? 2L : 0L);
    
    // Set proxy if configured
    if (!proxy.empty()) {
        LOG_DEBUG("Setting proxy: " + proxy);
        curl_easy_setopt(localCurl, CURLOPT_PROXY, proxy.c_str());
    }
    
    // Set custom headers
    struct curl_slist* headers = nullptr;
    for (const auto& header : customHeaders) {
        std::string headerStr = header.first + ": " + header.second;
        LOG_TRACE("Adding custom header: " + headerStr);
        headers = curl_slist_append(headers, headerStr.c_str());
    }
    if (headers) {
        curl_easy_setopt(localCurl, CURLOPT_HTTPHEADER, headers);
    }
    
    // Set write callback
    std::string responseData;
    LOG_TRACE("Setting write callback");
    curl_easy_setopt(localCurl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(localCurl, CURLOPT_WRITEDATA, &responseData);
    
    // Set progress callback if configured
    if (userProgressCallback) {
        LOG_DEBUG("Setting progress callback");
        curl_easy_setopt(localCurl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(localCurl, CURLOPT_XFERINFOFUNCTION, progressCallback);
        curl_easy_setopt(localCurl, CURLOPT_XFERINFODATA, &userProgressCallback);
    }
    
    // Perform the request
    LOG_INFO("Performing CURL request");
    CURLcode res = curl_easy_perform(localCurl);
    
    // Clean up headers
    if (headers) {
        LOG_TRACE("Cleaning up headers");
        curl_slist_free_all(headers);
    }
    
    if (res != CURLE_OK) {
        result.errorMessage = curl_easy_strerror(res);
        LOG_ERROR("CURL error: " + result.errorMessage);
        curl_easy_cleanup(localCurl);
        return result;
    }
    
    // Get status code
    long statusCode;
    curl_easy_getinfo(localCurl, CURLINFO_RESPONSE_CODE, &statusCode);
    result.statusCode = static_cast<int>(statusCode);
    LOG_INFO("===> HTTP Response status code: " + std::to_string(result.statusCode) + " for URL: " + url);
    
    // Log different status code categories
    if (result.statusCode >= 200 && result.statusCode < 300) {
        LOG_INFO("HTTP SUCCESS (" + std::to_string(result.statusCode) + "): " + url);
    } else if (result.statusCode >= 300 && result.statusCode < 400) {
        LOG_INFO("HTTP REDIRECT (" + std::to_string(result.statusCode) + "): " + url);
    } else if (result.statusCode >= 400 && result.statusCode < 500) {
        LOG_WARNING("HTTP CLIENT ERROR (" + std::to_string(result.statusCode) + "): " + url);
    } else if (result.statusCode >= 500) {
        LOG_ERROR("HTTP SERVER ERROR (" + std::to_string(result.statusCode) + "): " + url);
    }
    
    // Get content type
    char* contentType = nullptr;
    curl_easy_getinfo(localCurl, CURLINFO_CONTENT_TYPE, &contentType);
    if (contentType) {
        result.contentType = contentType;
        LOG_DEBUG("Response content type: " + result.contentType);
    }
    
    // Get final URL (after redirects)
    char* finalUrl = nullptr;
    curl_easy_getinfo(localCurl, CURLINFO_EFFECTIVE_URL, &finalUrl);
    if (finalUrl) {
        result.finalUrl = finalUrl;
        LOG_DEBUG("Final URL (after redirects): " + result.finalUrl);
    }
    
    result.content = std::move(responseData);
    result.success = true;
    LOG_INFO("Request successful, content size: " + std::to_string(result.content.size()) + " bytes");
    
    // Clean up the local curl handle
    curl_easy_cleanup(localCurl);
    LOG_DEBUG("Cleaned up local CURL handle");
    
    return result;
}

void PageFetcher::setProgressCallback(std::function<void(size_t, size_t)> callback) {
    LOG_DEBUG("PageFetcher::setProgressCallback called");
    userProgressCallback = std::move(callback);
}

void PageFetcher::setCustomHeaders(const std::vector<std::pair<std::string, std::string>>& headers) {
    LOG_DEBUG("PageFetcher::setCustomHeaders called with " + std::to_string(headers.size()) + " headers");
    customHeaders = headers;
}

void PageFetcher::setProxy(const std::string& proxyUrl) {
    LOG_DEBUG("PageFetcher::setProxy called with: " + proxyUrl);
    proxy = proxyUrl;
}

void PageFetcher::setVerifySSL(bool verify) {
    LOG_DEBUG("PageFetcher::setVerifySSL called with: " + std::string(verify ? "true" : "false"));
    verifySSL = verify;
}

size_t PageFetcher::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    std::string* responseData = static_cast<std::string*>(userp);
    size_t totalSize = size * nmemb;
    responseData->append(static_cast<char*>(contents), totalSize);
    // Too verbose for every chunk:
    // LOG_TRACE("CURL write callback received " + std::to_string(totalSize) + " bytes");
    return totalSize;
}

int PageFetcher::progressCallback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    // Mark parameters as unused to avoid compiler warnings
    (void)ultotal;  // Explicitly mark as unused
    (void)ulnow;    // Explicitly mark as unused
    
    auto* callback = static_cast<std::function<void(size_t, size_t)>*>(clientp);
    if (callback && *callback) {
        (*callback)(static_cast<size_t>(dlnow), static_cast<size_t>(dltotal));
    }
    return 0;
} 