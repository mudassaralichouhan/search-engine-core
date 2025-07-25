#include "PageFetcher.h"
#include "../../include/Logger.h"
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <thread>
#include <filesystem>
#include <regex>

PageFetcher::PageFetcher(const std::string& userAgent,
                         std::chrono::milliseconds timeout,
                         bool followRedirects,
                         size_t maxRedirects)
    : userAgent(userAgent)
    , timeout(timeout)
    , followRedirects(followRedirects)
    , maxRedirects(maxRedirects)
    , verifySSL(true)
    , spaRenderingEnabled(false) {
    LOG_DEBUG("PageFetcher constructor called with userAgent: " + userAgent);
    
    // Create a unique cookie jar file for this instance
    cookieJarPath = "/tmp/crawler_cookies_" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + ".txt";
    LOG_DEBUG("Cookie jar path: " + cookieJarPath);
    
    initCurl();
}

PageFetcher::~PageFetcher() {
    LOG_DEBUG("PageFetcher destructor called");
    cleanupCurl();
    
    // Clean up cookie jar file
    if (!cookieJarPath.empty()) {
        try {
            if (std::filesystem::exists(cookieJarPath)) {
                std::filesystem::remove(cookieJarPath);
                LOG_DEBUG("Removed cookie jar file: " + cookieJarPath);
            }
        } catch (const std::exception& e) {
            LOG_WARNING("Failed to remove cookie jar file: " + std::string(e.what()));
        }
    }
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
    
    // Set connection timeout to 1 second
    LOG_DEBUG("Setting connection timeout: 1000ms");
    curl_easy_setopt(localCurl, CURLOPT_CONNECTTIMEOUT_MS, 1000L);
    
    // Set low-speed time and limit to abort slow connections
    LOG_DEBUG("Setting low speed time: 2 seconds");
    curl_easy_setopt(localCurl, CURLOPT_LOW_SPEED_TIME, 2L);
    curl_easy_setopt(localCurl, CURLOPT_LOW_SPEED_LIMIT, 1L);
    
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

    // Set cookie handling
    LOG_DEBUG("Setting cookie handling");
    curl_easy_setopt(localCurl, CURLOPT_COOKIEFILE, cookieJarPath.c_str());
    curl_easy_setopt(localCurl, CURLOPT_COOKIEJAR, cookieJarPath.c_str());
    
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
    
    // Set success based on HTTP status code
    // 2xx status codes are considered successful
    result.success = (result.statusCode >= 200 && result.statusCode < 300);
    
    if (result.success) {
        LOG_INFO("Request successful, content size: " + std::to_string(result.content.size()) + " bytes");
    } else {
        LOG_INFO("Request completed with error status " + std::to_string(result.statusCode) + ", content size: " + std::to_string(result.content.size()) + " bytes");
    }
    
    // Clean up the local curl handle
    curl_easy_cleanup(localCurl);
    LOG_DEBUG("Cleaned up local CURL handle");
    
    // Check if this is a SPA and render if enabled
    if (result.success && spaRenderingEnabled && isSpaPage(result.content, url)) {
        LOG_INFO("SPA detected, attempting to render with browserless: " + url);
        
        if (browserlessClient) {
            try {
                auto renderResult = browserlessClient->renderUrl(url, static_cast<int>(timeout.count()));
                if (renderResult.success) {
                    LOG_INFO("Successfully rendered SPA, content size: " + std::to_string(renderResult.html.size()) + " bytes");
                    result.content = renderResult.html;
                    result.statusCode = renderResult.status_code;
                } else {
                    LOG_WARNING("Failed to render SPA: " + renderResult.error + ", using original content");
                }
            } catch (const std::exception& e) {
                LOG_ERROR("Exception during SPA rendering: " + std::string(e.what()) + ", using original content");
            }
        } else {
            LOG_WARNING("BrowserlessClient not available for SPA rendering");
        }
    }
    
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

void PageFetcher::setSpaRendering(bool enable, const std::string& browserless_url) {
    LOG_DEBUG("PageFetcher::setSpaRendering called with enable: " + std::string(enable ? "true" : "false") + ", url: " + browserless_url);
    spaRenderingEnabled = enable;
    
    if (enable) {
        browserlessClient = std::make_unique<BrowserlessClient>(browserless_url);
        LOG_INFO("BrowserlessClient initialized for SPA rendering");
    } else {
        browserlessClient.reset();
        LOG_INFO("BrowserlessClient disabled");
    }
}

bool PageFetcher::isSpaPage(const std::string& html, const std::string& url) {
    // Convert to lowercase for case-insensitive matching
    std::string lowerHtml = html;
    std::transform(lowerHtml.begin(), lowerHtml.end(), lowerHtml.begin(), ::tolower);
    
    // Common SPA indicators
    std::vector<std::string> spaIndicators = {
        "react", "vue", "angular", "ember", "backbone", "svelte",
        "single-page", "client-side", "javascript framework",
        "data-reactroot", "ember-", "svelte-",
        "window.__initial_state__", "window.__preloaded_state__",
        "window.__data__", "window.__props__",
        // Next.js indicators
        "next-head-count", "data-n-g", "data-n-p", "/_next/static/",
        "next.js", "nextjs", "next/", "__next__",
        // Nuxt.js indicators
        "nuxt", "nuxtjs", "_nuxt/", "data-nuxt-",
        // Gatsby indicators
        "gatsby", "gatsbyjs", "___gatsby", "gatsby-",
        // Modern SSR/SPA frameworks
        "remix", "sveltekit", "astro", "qwik",
        // React patterns
        "react-", "reactjs", "react-dom", "react/",
        // Vue patterns
        "vue-", "vuejs", "vue-router", "vuex",
        // Angular patterns
        "angular-", "angularjs", "angular/",
        // State management
        "redux", "mobx", "zustand", "recoil", "jotai",
        // Build tools that indicate SPAs
        "webpack", "vite", "parcel", "rollup", "esbuild"
    };
    
    // Check for SPA frameworks and patterns with more precise matching
    for (const auto& indicator : spaIndicators) {
        // Skip very short patterns that might cause false positives
        if (indicator.length() < 3) continue;
        
        // Use more specific patterns for common false positives
        if (indicator == "angular" && lowerHtml.find("angular") != std::string::npos) {
            // Check if it's actually Angular framework, not just the word "angular"
            if (lowerHtml.find("angularjs") != std::string::npos || 
                lowerHtml.find("angular/") != std::string::npos) {
                LOG_DEBUG("SPA detected by Angular indicator in URL: " + url);
                return true;
            }
        } else if (indicator == "solid" && lowerHtml.find("solid") != std::string::npos) {
            // Check if it's actually SolidJS, not just the word "solid"
            if (lowerHtml.find("solidjs") != std::string::npos ||
                lowerHtml.find("solid-") != std::string::npos) {
                LOG_DEBUG("SPA detected by SolidJS indicator in URL: " + url);
                return true;
            }
        } else if (indicator == "spa" && lowerHtml.find("spa") != std::string::npos) {
            // Check if it's actually SPA framework, not just the word "spa"
            if (lowerHtml.find("single-page") != std::string::npos ||
                lowerHtml.find("spa framework") != std::string::npos) {
                LOG_DEBUG("SPA detected by SPA framework indicator in URL: " + url);
                return true;
            }
        } else if (lowerHtml.find(indicator) != std::string::npos) {
            LOG_DEBUG("SPA detected by indicator: " + indicator + " in URL: " + url);
            return true;
        }
    }
    
    // Check for modern SPA patterns
    std::regex scriptRegex(R"(<script[^>]*src[^>]*>)");
    std::regex bodyContentRegex(R"(<body[^>]*>.*?</body>)", std::regex_constants::icase);
    std::regex divRegex(R"(<div[^>]*id[^>]*>)");
    std::regex appRegex(R"(<div[^>]*id\s*=\s*["'](app|root|main|#app|#root|#main)["'][^>]*>)", std::regex_constants::icase);
    
    auto scriptMatches = std::distance(std::sregex_iterator(lowerHtml.begin(), lowerHtml.end(), scriptRegex), std::sregex_iterator());
    auto bodyMatch = std::regex_search(lowerHtml, bodyContentRegex);
    auto appMatches = std::distance(std::sregex_iterator(lowerHtml.begin(), lowerHtml.end(), appRegex), std::sregex_iterator());
    
    // Modern SPA detection logic
    bool hasAppRoot = appMatches > 0;
    bool hasMultipleScripts = scriptMatches > 3;
    bool hasMinimalContent = false;
    
    // Check for minimal body content (common in SPAs)
    if (bodyMatch) {
        std::smatch bodyMatchResult;
        if (std::regex_search(lowerHtml, bodyMatchResult, bodyContentRegex)) {
            std::string bodyContent = bodyMatchResult.str();
            // Remove script tags from body content for length check
            std::regex scriptTagRegex(R"(<script[^>]*>.*?</script>)", std::regex_constants::icase);
            bodyContent = std::regex_replace(bodyContent, scriptTagRegex, "");
            
            // If body content is minimal, likely SPA
            hasMinimalContent = bodyContent.length() < 1000;
        }
    }
    
    // Check for modern SPA characteristics
    if (hasAppRoot || (hasMultipleScripts && hasMinimalContent) || scriptMatches > 8) {
        LOG_DEBUG("SPA detected by modern patterns in URL: " + url);
        return true;
    }
    
    // Check for hydration patterns (common in SSR SPAs)
    if (lowerHtml.find("hydration") != std::string::npos || 
        lowerHtml.find("hydrate") != std::string::npos ||
        lowerHtml.find("client-side") != std::string::npos) {
        LOG_DEBUG("SPA detected by hydration patterns in URL: " + url);
        return true;
    }
    
    return false;
} 