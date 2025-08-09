#include "BrowserlessClient.h"
#include "../../include/Logger.h"
#include "../../include/crawler/CrawlLogger.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

using json = nlohmann::json;

class BrowserlessClient::Impl {
public:
    Impl(const std::string& browserless_url) 
        : browserless_url_(browserless_url), 
          user_agent_("Hatefbot/1.0") {
        
        // Initialize CURL
        curl_global_init(CURL_GLOBAL_ALL);
        curl_ = curl_easy_init();
        
        if (!curl_) {
            LOG_ERROR("Failed to initialize CURL for BrowserlessClient");
        }
    }
    
    ~Impl() {
        if (curl_) {
            curl_easy_cleanup(curl_);
        }
        curl_global_cleanup();
    }
    
    BrowserlessRenderResult renderUrl(const std::string& url, int timeout_ms, bool wait_for_network_idle) {
        BrowserlessRenderResult result;
        result.success = false;
        result.status_code = 0;
        
        auto start_time = std::chrono::steady_clock::now();
        
        LOG_INFO("Starting headless browser rendering for: " + url);
        CrawlLogger::broadcastLog("🤖 Starting headless browser rendering for: " + url, "info");
        
        try {
            if (!curl_) {
                result.error = "CURL not initialized";
                return result;
            }
            
            // Prepare the request to browserless
            std::string browserless_endpoint = browserless_url_ + "/content";
            
            // Create JSON payload for browserless
            json payload = {
                {"url", url},
                {"waitFor", wait_for_network_idle ? 5000 : 2000}
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
            
            // Set up CURL options
            curl_easy_reset(curl_);
            curl_easy_setopt(curl_, CURLOPT_URL, browserless_endpoint.c_str());
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, json_payload.c_str());
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, json_payload.length());
            curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, timeout_ms);
            curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT_MS, 5000);  // Reduced connection timeout
            curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_LIMIT, 1000L);   // Minimum 1KB/s
            curl_easy_setopt(curl_, CURLOPT_LOW_SPEED_TIME, 10L);      // For 10 seconds
            
            // Set headers
            struct curl_slist* headers = nullptr;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, ("User-Agent: " + user_agent_).c_str());
            curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
            
            // Set up response handling
            std::string response;
            curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
            curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
            
            // Perform the request
            CURLcode res = curl_easy_perform(curl_);
            
            // Clean up headers
            curl_slist_free_all(headers);
            
            if (res != CURLE_OK) {
                result.error = "CURL error: " + std::string(curl_easy_strerror(res));
                if (res == CURLE_OPERATION_TIMEDOUT || res == CURLE_COULDNT_CONNECT) {
                    LOG_WARNING("Browserless timeout/connection failure for: " + url + " - " + result.error);
                    CrawlLogger::broadcastLog("⚠️ Browserless unavailable for: " + url + " - falling back to static HTML", "warning");
                } else {
                    LOG_ERROR("Browserless CURL error for: " + url + " - " + result.error);
                }
                return result;
            }
            
            // Get HTTP status code
            long http_code = 0;
            curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
            result.status_code = static_cast<int>(http_code);
            
            if (http_code == 200) {
                result.html = response;
                result.success = true;
                LOG_INFO("Successfully rendered page via browserless: " + url + ", size: " + std::to_string(response.size()) + " bytes");
                CrawlLogger::broadcastLog("✅ Headless browser rendering completed for: " + url + " (Size: " + std::to_string(response.size()) + " bytes)", "info");
            } else {
                result.error = "Browserless returned HTTP " + std::to_string(http_code) + ": " + response;
                LOG_ERROR("Browserless error: " + result.error);
                CrawlLogger::broadcastLog("❌ Headless browser rendering failed for: " + url + " - HTTP " + std::to_string(http_code), "error");
            }
            
        } catch (const std::exception& e) {
            result.error = "Exception during rendering: " + std::string(e.what());
            LOG_ERROR("Exception in BrowserlessClient::renderUrl: " + result.error);
            CrawlLogger::broadcastLog("❌ Headless browser rendering exception for: " + url + " - " + std::string(e.what()), "error");
        }
        
        auto end_time = std::chrono::steady_clock::now();
        result.render_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        return result;
    }
    
    bool isAvailable() {
        try {
            if (!curl_) return false;
            
            // Try a simple health check
            std::string health_url = browserless_url_ + "/health";
            
            curl_easy_reset(curl_);
            curl_easy_setopt(curl_, CURLOPT_URL, health_url.c_str());
            curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, 5000);
            curl_easy_setopt(curl_, CURLOPT_NOBODY, 1L);
            
            CURLcode res = curl_easy_perform(curl_);
            if (res != CURLE_OK) return false;
            
            long http_code = 0;
            curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &http_code);
            return http_code == 200;
            
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
    CURL* curl_;
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

bool BrowserlessClient::isAvailable() {
    return pImpl->isAvailable();
}

void BrowserlessClient::setHeaders(const std::map<std::string, std::string>& headers) {
    pImpl->setHeaders(headers);
}

void BrowserlessClient::setUserAgent(const std::string& user_agent) {
    pImpl->setUserAgent(user_agent);
} 