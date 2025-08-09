#pragma once
#include "WebSocketHandler.h"
#include <thread>
#include <atomic>

class DateTimeWebSocketHandler : public WebSocketHandler {
public:
    DateTimeWebSocketHandler();
    ~DateTimeWebSocketHandler() override;
    void registerEndpoint(uWS::App& app) override;

private:
    static constexpr const char* DATETIME_TOPIC = "datetime";
    std::atomic<bool> running;
    std::thread broadcastThread;
    void broadcastLoop();
}; 