// Deprecated include path: "include/BrowserlessClient.h"
// Prefer: "include/search_engine/crawler/BrowserlessClient.h"
#pragma once

#include <string>
#include <memory>
#include <future>
#include <map>
#include <chrono>
#include <curl/curl.h>

struct BrowserlessRenderResult {
    bool success;
    std::string html;
    std::string error;
    int status_code;
    std::chrono::milliseconds render_time;
};

class BrowserlessClient {
public:
    BrowserlessClient(const std::string& browserless_url);
    ~BrowserlessClient();
    
    BrowserlessRenderResult renderUrl(const std::string& url, 
                                     int timeout_ms = 60000, 
                                     bool wait_for_network_idle = false);
    bool isAvailable();
    void setHeaders(const std::map<std::string, std::string>& headers);
    void setUserAgent(const std::string& user_agent);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};