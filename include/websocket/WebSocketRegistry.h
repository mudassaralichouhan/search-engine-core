#pragma once
#include <vector>
#include <memory>
#include "WebSocketHandler.h"

class WebSocketRegistry {
public:
    void addHandler(std::shared_ptr<WebSocketHandler> handler) {
        handlers.push_back(handler);
    }
    void registerAll(uWS::App& app) {
        for (auto& handler : handlers) {
            handler->registerEndpoint(app);
        }
    }
private:
    std::vector<std::shared_ptr<WebSocketHandler>> handlers;
}; 