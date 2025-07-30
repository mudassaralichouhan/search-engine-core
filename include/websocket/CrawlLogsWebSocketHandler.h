#pragma once
#include "WebSocketHandler.h"
#include <unordered_set>
#include <mutex>
#include <string>

class CrawlLogsWebSocketHandler : public WebSocketHandler {
public:
    CrawlLogsWebSocketHandler();
    ~CrawlLogsWebSocketHandler() override = default;
    void registerEndpoint(uWS::App& app) override;
    
    // Static method to broadcast log messages to all connected clients
    static void broadcastLog(const std::string& message, const std::string& level = "info");
    
private:
    static std::unordered_set<uWS::WebSocket<false, true, PerSocketData>*> clients;
    static std::mutex clientsMutex;
    
    void onOpen(uWS::WebSocket<false, true, PerSocketData>* ws);
    void onClose(uWS::WebSocket<false, true, PerSocketData>* ws, int code, std::string_view message);
    
    // Helper function to get current timestamp
    static std::string getCurrentTimestamp();
};