#pragma once
#include "WebSocketHandler.h"
#include <string>
#include <chrono>
#include <deque>
#include <mutex>

class CrawlLogsWebSocketHandler : public WebSocketHandler {
public:
    CrawlLogsWebSocketHandler();
    ~CrawlLogsWebSocketHandler() override = default;
    void registerEndpoint(uWS::App& app) override;
    
    // Static method to broadcast log messages to all connected clients using pub/sub
    static void broadcastLog(const std::string& message, const std::string& level = "info");
    
    // Session-specific broadcasting
    static void broadcastToSession(const std::string& sessionId, const std::string& message, const std::string& level = "info");
    
private:
    static uWS::App* globalApp; // Reference to app for pub/sub broadcasting
    static uWS::Loop* serverLoop; // uWS server loop captured on registration thread
    static constexpr const char* CRAWL_LOGS_ADMIN_TOPIC = "crawl-logs-admin";
    static constexpr const char* CRAWL_LOGS_SESSION_PREFIX = "crawl-logs-session-";
    
    // Rate limiting for message broadcasting
    static std::deque<std::chrono::steady_clock::time_point> messageTimes;
    static std::mutex rateLimitMutex;
    static constexpr size_t MAX_MESSAGES_PER_SECOND = 50;
    static constexpr std::chrono::milliseconds RATE_LIMIT_WINDOW{1000};
    
    void onOpen(uWS::WebSocket<false, true, PerSocketData>* ws);
    void onMessage(uWS::WebSocket<false, true, PerSocketData>* ws, std::string_view message, uWS::OpCode opCode);
    void onDrain(uWS::WebSocket<false, true, PerSocketData>* ws);
    void onClose(uWS::WebSocket<false, true, PerSocketData>* ws, int code, std::string_view message);
    
    // Helper functions
    static std::string getCurrentTimestamp();
    static bool shouldThrottleMessage();
    static std::string parseSessionIdFromQuery(const std::string& query);
};