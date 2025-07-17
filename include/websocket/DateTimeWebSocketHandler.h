#pragma once
#include "WebSocketHandler.h"
#include <unordered_set>
#include <thread>
#include <atomic>

class DateTimeWebSocketHandler : public WebSocketHandler {
public:
    DateTimeWebSocketHandler();
    ~DateTimeWebSocketHandler() override;
    void registerEndpoint(uWS::App& app) override;

private:
    std::unordered_set<uWS::WebSocket<false, true, PerSocketData>*> clients;
    std::atomic<bool> running;
    std::thread broadcastThread;
    void broadcastLoop();
}; 