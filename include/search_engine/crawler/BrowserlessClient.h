#pragma once

// Wrapper header to expose BrowserlessClient under the public search_engine path
#include "../../BrowserlessClient.h"

namespace search_engine::crawler {

// Simple transport selector wrapper
class BrowserlessTransportClient {
public:
    explicit BrowserlessTransportClient(const std::string& baseUrl,
                                        bool useWebsocket,
                                        size_t poolSize = 0);
    ~BrowserlessTransportClient();

    BrowserlessRenderResult renderUrl(const std::string& url,
                                      int timeout_ms = 60000,
                                      bool wait_for_network_idle = false);

    bool isAvailable();

    void setHeaders(const std::map<std::string, std::string>& headers);
    void setUserAgent(const std::string& user_agent);

private:
    std::string baseUrl_;
    bool useWebsocket_;
    std::unique_ptr<BrowserlessClient> httpClient_;
    // TODO: add websocket pool when implemented
};

}


