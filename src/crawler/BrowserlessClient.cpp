#include "BrowserlessClient.h"
#include "../../include/Logger.h"
#include "../../include/crawler/CrawlLogger.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>
#include "../../include/search_engine/common/UrlSanitizer.h"
#include "../../include/search_engine/crawler/BrowserlessClient.h"

using json = nlohmann::json;

class BrowserlessClient::Impl {
public:
    Impl(const std::string& browserless_url) 
        : browserless_url_(browserless_url), 
          user_agent_("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36") {
        
        // Initialize CURL globally (thread-safe, can be called multiple times)
        curl_global_init(CURL_GLOBAL_ALL);
    }
    
    ~Impl() {
        // Note: curl_global_cleanup should only be called when no other threads are using libcurl
        // In a multi-threaded server, we typically don't call it
    }
    
    BrowserlessRenderResult renderUrl(const std::string& url, int timeout_ms, bool wait_for_network_idle) {
        BrowserlessRenderResult result;
        result.success = false;
        result.status_code = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        auto start_sys = std::chrono::system_clock::now();
        
        using namespace search_engine::common;
        const std::string cleanedUrl = sanitizeUrl(url);
        LOG_INFO("Starting headless browser rendering for: " + cleanedUrl);
        CrawlLogger::broadcastLog("🤖 Starting headless browser rendering for: " + cleanedUrl, "info");
        
        try {
            // Create a local CURL handle for thread safety
            CURL* curl = curl_easy_init();
            if (!curl) {
                result.error = "Failed to create CURL handle";
                LOG_ERROR("Failed to create local CURL handle for BrowserlessClient");
                return result;
            }
            
            // Prepare the request to browserless (don't sanitize the endpoint URL)
            std::string browserless_endpoint = browserless_url_ + "/content";
            
            // Create JSON payload for browserless (best-practice defaults)
            // Use 20s for network-idle waits, 5s for simple waits
            json payload = {
                {"url", cleanedUrl},
                {"waitFor", wait_for_network_idle ? 20000 : 5000},
                {"rejectResourceTypes", json::array({"image", "media", "font"})}
            };
            
            // Add custom headers if any
            if (!custom_headers_.empty()) {
                json headers = json::object();
                for (const auto& [key, value] : custom_headers_) {
                    headers[key] = value;
                }
                payload["headers"] = headers;
            }
            
            std::string json_payload = payload.dump();
            
            // Debug logging
            LOG_DEBUG("Browserless endpoint: " + browserless_endpoint);
            LOG_DEBUG("JSON payload size: " + std::to_string(json_payload.size()) + " bytes");
            LOG_DEBUG("JSON payload: " + json_payload);
            
            // Set up CURL options
            curl_easy_setopt(curl, CURLOPT_URL, browserless_endpoint.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(json_payload.size()));
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(timeout_ms));
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 5000L);  // Reduced connection timeout
            curl_easy_setopt(curl, CURLOPT_PROTOCOLS, static_cast<long>(CURLPROTO_HTTP | CURLPROTO_HTTPS));
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
            char errbuf[CURL_ERROR_SIZE] = {0};
            curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
            // Keep connection alive
            curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
            curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 30L);
            curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 15L);
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
            
            // Set headers
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, ("User-Agent: " + user_agent_).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            
            // Set up response handling
            std::string response;
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            
            // Perform the request in an isolated thread to avoid interaction with other libs
            CURLcode res = CURLE_OK;
            std::exception_ptr perform_exc;
            std::thread curlThread([&]() {
                res = curl_easy_perform(curl);
            });
            curlThread.join();
            
            // Clean up headers
            curl_slist_free_all(headers);
            
            if (res != CURLE_OK) {
                result.error = "CURL error: " + std::string(curl_easy_strerror(res)) + " | errbuf=" + errbuf + " | url_hex=" + hexDump(cleanedUrl);
                // Try to capture HTTP code and any partial HTML
                long http_code_partial = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code_partial);
                result.status_code = static_cast<int>(http_code_partial);
                if (!response.empty()) {
                    result.html = response; // partial but possibly useful HTML
                }
                if (res == CURLE_OPERATION_TIMEDOUT || res == CURLE_COULDNT_CONNECT) {
                    auto end_sys_err = std::chrono::system_clock::now();
                    auto end_steady_err = std::chrono::steady_clock::now();
                    long long start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(start_sys.time_since_epoch()).count();
                    long long end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_sys_err.time_since_epoch()).count();
                    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_steady_err - start_time).count();
                    std::string msg =
                        "⚠️ Browserless unavailable for: " + cleanedUrl +
                        " - falling back to static HTML" +
                        " [error=" + result.error +
                        ", start_ts_ms=" + std::to_string(start_ms) +
                        ", end_ts_ms=" + std::to_string(end_ms) +
                        ", duration_ms=" + std::to_string(elapsed_ms) +
                        ", ~" + std::to_string(static_cast<double>(elapsed_ms) / 1000.0) + "s" +
                        ", timeout_ms=" + std::to_string(timeout_ms) +
                        ", received_bytes=" + std::to_string(response.size()) + 
                        (http_code_partial > 0 ? ", http_status=" + std::to_string(http_code_partial) : "") + 
                        "]";
                    LOG_WARNING(msg);
                    CrawlLogger::broadcastLog(msg, "warning");
                } else {
                    LOG_ERROR("Browserless CURL error for: " + cleanedUrl + " - " + result.error);
                }
                curl_easy_cleanup(curl);
                return result;
            }
            
            // Get HTTP status code
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            result.status_code = static_cast<int>(http_code);
            
            if (http_code == 200) {
                result.html = response;
                result.success = true;
                LOG_INFO("Successfully rendered page via browserless: " + cleanedUrl + ", size: " + std::to_string(response.size()) + " bytes");
                CrawlLogger::broadcastLog("✅ Headless browser rendering completed for: " + cleanedUrl + " (Size: " + std::to_string(response.size()) + " bytes)", "info");
            } else {
                result.error = "Browserless returned HTTP " + std::to_string(http_code) + ": " + response;
                LOG_ERROR("Browserless error: " + result.error);
                CrawlLogger::broadcastLog("❌ Headless browser rendering failed for: " + cleanedUrl + " - HTTP " + std::to_string(http_code), "error");
            }
            
            // Clean up curl handle
            curl_easy_cleanup(curl);
            
        } catch (const std::exception& e) {
            result.error = "Exception during rendering: " + std::string(e.what());
            LOG_ERROR("Exception in BrowserlessClient::renderUrl: " + result.error);
            CrawlLogger::broadcastLog("❌ Headless browser rendering exception for: " + cleanedUrl + " - " + std::string(e.what()), "error");
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto end_sys = std::chrono::system_clock::now();
        result.render_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        // Also broadcast detailed timing for success cases
        long long start_ms = std::chrono::duration_cast<std::chrono::milliseconds>(start_sys.time_since_epoch()).count();
        long long end_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_sys.time_since_epoch()).count();
        CrawlLogger::broadcastLog(
            "🕒 Browserless render timing for: " + url +
            " [start_ts_ms=" + std::to_string(start_ms) +
            ", end_ts_ms=" + std::to_string(end_ms) +
            ", duration_ms=" + std::to_string(result.render_time.count()) +
            ", ~" + std::to_string(static_cast<double>(result.render_time.count()) / 1000.0) + "s]",
            "debug");
        
        return result;
    }
    
    bool isAvailable() {
        try {
            // Create a local CURL handle for thread safety
            CURL* curl = curl_easy_init();
            if (!curl) return false;
            
            // Try a simple health check
            std::string health_url = browserless_url_ + "/health";
            
            curl_easy_setopt(curl, CURLOPT_URL, health_url.c_str());
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 5000L);
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
            curl_easy_setopt(curl, CURLOPT_PROTOCOLS, static_cast<long>(CURLPROTO_HTTP | CURLPROTO_HTTPS));
            curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
            
            CURLcode res = curl_easy_perform(curl);
            bool available = false;
            
            if (res == CURLE_OK) {
                long http_code = 0;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
                available = (http_code == 200);
            }
            
            curl_easy_cleanup(curl);
            return available;
            
        } catch (...) {
            return false;
        }
    }
    
    void setHeaders(const std::map<std::string, std::string>& headers) {
        custom_headers_ = headers;
    }
    
    void setUserAgent(const std::string& user_agent) {
        user_agent_ = user_agent;
    }
    
private:
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
        std::string* response = static_cast<std::string*>(userp);
        size_t totalSize = size * nmemb;
        response->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }
    
    std::string browserless_url_;
    std::map<std::string, std::string> custom_headers_;
    std::string user_agent_;
};

// Public interface implementation
BrowserlessClient::BrowserlessClient(const std::string& browserless_url)
    : pImpl(std::make_unique<Impl>(browserless_url)) {}

BrowserlessClient::~BrowserlessClient() = default;

BrowserlessRenderResult BrowserlessClient::renderUrl(const std::string& url, 
                                                    int timeout_ms, 
                                                    bool wait_for_network_idle) {
    return pImpl->renderUrl(url, timeout_ms, wait_for_network_idle);
}

// Transport wrapper (HTTP now; WS placeholder)
namespace search_engine::crawler {

BrowserlessTransportClient::BrowserlessTransportClient(const std::string& baseUrl,
                                                       bool useWebsocket,
                                                       size_t /*poolSize*/)
    : baseUrl_(baseUrl), useWebsocket_(useWebsocket) {
    // For now, always initialize HTTP client; WS to be added later
    httpClient_ = std::make_unique<::BrowserlessClient>(baseUrl_);
}

BrowserlessTransportClient::~BrowserlessTransportClient() = default;

BrowserlessRenderResult BrowserlessTransportClient::renderUrl(const std::string& url,
                                                              int timeout_ms,
                                                              bool wait_for_network_idle) {
    // If WS enabled and implemented, route to WS pool. Fallback to HTTP
    return httpClient_->renderUrl(url, timeout_ms, wait_for_network_idle);
}

bool BrowserlessTransportClient::isAvailable() {
    return httpClient_->isAvailable();
}

void BrowserlessTransportClient::setHeaders(const std::map<std::string, std::string>& headers) {
    httpClient_->setHeaders(headers);
}

void BrowserlessTransportClient::setUserAgent(const std::string& user_agent) {
    httpClient_->setUserAgent(user_agent);
}

} // namespace search_engine::crawler

bool BrowserlessClient::isAvailable() {
    return pImpl->isAvailable();
}

void BrowserlessClient::setHeaders(const std::map<std::string, std::string>& headers) {
    pImpl->setHeaders(headers);
}

void BrowserlessClient::setUserAgent(const std::string& user_agent) {
    pImpl->setUserAgent(user_agent);
} 