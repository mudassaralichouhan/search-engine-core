#include "websocket/CrawlLogsWebSocketHandler.h"
#include "../include/Logger.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>

// Static member definitions
std::unordered_set<uWS::WebSocket<false, true, PerSocketData>*> CrawlLogsWebSocketHandler::clients;
std::mutex CrawlLogsWebSocketHandler::clientsMutex;

CrawlLogsWebSocketHandler::CrawlLogsWebSocketHandler() {
    LOG_INFO("CrawlLogsWebSocketHandler initialized");
}

void CrawlLogsWebSocketHandler::registerEndpoint(uWS::App& app) {
    app.ws<PerSocketData>("/crawl-logs", {
        .open = [this](auto* ws) { onOpen(ws); },
        .close = [this](auto* ws, int code, std::string_view message) { onClose(ws, code, message); }
    });
    
    LOG_INFO("CrawlLogsWebSocketHandler registered endpoint: /crawl-logs");
}

void CrawlLogsWebSocketHandler::onOpen(uWS::WebSocket<false, true, PerSocketData>* ws) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    clients.insert(ws);
    
    LOG_INFO("Client connected to crawl logs WebSocket. Total clients: " + std::to_string(clients.size()));
    
    // Send welcome message
    nlohmann::json welcomeMsg = {
        {"level", "info"},
        {"message", "Connected to crawl logs WebSocket"},
        {"timestamp", getCurrentTimestamp()}
    };
    
    ws->send(welcomeMsg.dump(), uWS::OpCode::TEXT);
}

void CrawlLogsWebSocketHandler::onClose(uWS::WebSocket<false, true, PerSocketData>* ws, int code, std::string_view message) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    clients.erase(ws);
    
    LOG_INFO("Client disconnected from crawl logs WebSocket. Total clients: " + std::to_string(clients.size()));
}

void CrawlLogsWebSocketHandler::broadcastLog(const std::string& message, const std::string& level) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    
    if (clients.empty()) {
        return; // No clients connected
    }
    
    // Create JSON message with timestamp
    nlohmann::json logMsg = {
        {"level", level},
        {"message", message},
        {"timestamp", getCurrentTimestamp()}
    };
    
    std::string jsonString = logMsg.dump();
    
    // Broadcast to all connected clients
    for (auto it = clients.begin(); it != clients.end();) {
        auto* ws = *it;
        try {
            ws->send(jsonString, uWS::OpCode::TEXT);
            ++it;
        } catch (const std::exception& e) {
            LOG_WARNING("Failed to send message to WebSocket client, removing from list: " + std::string(e.what()));
            it = clients.erase(it);
        }
    }
}

// Helper function to get current timestamp
std::string CrawlLogsWebSocketHandler::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
    return ss.str();
}