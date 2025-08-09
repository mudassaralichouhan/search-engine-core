#include "websocket/CrawlLogsWebSocketHandler.h"
#include "../include/Logger.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <iostream>

// Static member definitions
uWS::App* CrawlLogsWebSocketHandler::globalApp = nullptr;
uWS::Loop* CrawlLogsWebSocketHandler::serverLoop = nullptr;
std::deque<std::chrono::steady_clock::time_point> CrawlLogsWebSocketHandler::messageTimes;
std::mutex CrawlLogsWebSocketHandler::rateLimitMutex;

CrawlLogsWebSocketHandler::CrawlLogsWebSocketHandler() {
    LOG_INFO("CrawlLogsWebSocketHandler initialized");
}

void CrawlLogsWebSocketHandler::registerEndpoint(uWS::App& app) {
    // Store global app reference for pub/sub broadcasting
    globalApp = &app;
    // Capture the server thread's loop to use for deferred publishes
    serverLoop = uWS::Loop::get();
    
    app.ws<PerSocketData>("/crawl-logs", {
        .open = [this](auto* ws) { onOpen(ws); },
        .message = [this](auto* ws, std::string_view message, uWS::OpCode opCode) { 
            // Handle incoming messages (heartbeat, etc.)
            onMessage(ws, message, opCode);
        },
        .drain = [this](auto* ws) {
            // Called when backpressure is relieved
            onDrain(ws);
        },
        .close = [this](auto* ws, int code, std::string_view message) { 
            onClose(ws, code, message); 
        }
    });
    
    LOG_INFO("CrawlLogsWebSocketHandler registered endpoint: /crawl-logs with session-aware pub/sub pattern");
}

void CrawlLogsWebSocketHandler::onOpen(uWS::WebSocket<false, true, PerSocketData>* ws) {
    // Mark socket as open for lifecycle management
    ws->getUserData()->open = true;
    
    // Default to admin access (for backward compatibility)
    // Client will send sessionId message to switch to session-specific logs
    ws->getUserData()->isAdmin = true;
    ws->subscribe(CRAWL_LOGS_ADMIN_TOPIC);
    
    std::cout << "[WS-DEBUG] NEW CLIENT CONNECTED! Subscribed to " << CRAWL_LOGS_ADMIN_TOPIC << " (admin access)" << std::endl;
    LOG_INFO("Client connected to crawl logs WebSocket with admin access");
    
    // Send welcome message directly (safe in lifecycle callback)
    nlohmann::json welcomeMsg = {
        {"level", "info"},
        {"message", "Connected to crawl logs WebSocket"},
        {"timestamp", getCurrentTimestamp()}
    };
    
    std::string welcomeJson = welcomeMsg.dump();
    if (!ws->send(welcomeJson, uWS::OpCode::TEXT)) {
        // Backpressure detected, will retry on drain
        std::cout << "[WS-DEBUG] Welcome message backpressure, will retry on drain" << std::endl;
    } else {
        std::cout << "[WS-DEBUG] Welcome message sent successfully" << std::endl;
    }
}

void CrawlLogsWebSocketHandler::onMessage(uWS::WebSocket<false, true, PerSocketData>* ws, std::string_view message, uWS::OpCode opCode) {
    // Check socket lifecycle state
    if (!ws->getUserData()->open) return;
    
    std::cout << "[WS-DEBUG] RECEIVED MESSAGE: '" << message << "' (OpCode: " << (int)opCode << ")" << std::endl;
    
    // Handle heartbeat or client messages
    if (message == "ping") {
        std::cout << "[WS-DEBUG] Handling PING - sending PONG response" << std::endl;
        // Safe to send synchronously in lifecycle callback
        if (!ws->send("pong", uWS::OpCode::TEXT)) {
            // Backpressure detected, will retry on drain
            std::cout << "[WS-DEBUG] PONG backpressure, will retry on drain" << std::endl;
        } else {
            std::cout << "[WS-DEBUG] PONG sent successfully" << std::endl;
        }
    }
    // Explicitly (re)subscribe to admin topic MUST be handled before generic subscribe
    else if (message == "subscribe:admin") {
        // If previously on a session topic, unsubscribe from it
        if (!ws->getUserData()->userTopic.empty()) {
            ws->unsubscribe(ws->getUserData()->userTopic);
            ws->getUserData()->userTopic.clear();
        }
        ws->subscribe(CRAWL_LOGS_ADMIN_TOPIC);
        ws->getUserData()->isAdmin = true;
        ws->getUserData()->sessionId.clear();
        
        std::cout << "[WS-DEBUG] Client subscribed to admin topic: " << CRAWL_LOGS_ADMIN_TOPIC << std::endl;
        LOG_INFO("Client switched to admin logs topic");
        
        nlohmann::json confirmMsg = {
            {"level", "info"},
            {"message", "Subscribed to admin logs"},
            {"timestamp", getCurrentTimestamp()}
        };
        std::string confirmJson = confirmMsg.dump();
        ws->send(confirmJson, uWS::OpCode::TEXT);
    }
    // Handle session subscription requests
    else if (message.length() > 10 && message.substr(0, 10) == "subscribe:") {
        std::string sessionId = std::string(message.substr(10)); // Remove "subscribe:" prefix
        if (!sessionId.empty()) {
            // Unsubscribe from admin topic
            ws->unsubscribe(CRAWL_LOGS_ADMIN_TOPIC);
            
            // Subscribe to session-specific topic
            std::string sessionTopic = std::string(CRAWL_LOGS_SESSION_PREFIX) + sessionId;
            ws->subscribe(sessionTopic);
            
            // Update user data
            ws->getUserData()->isAdmin = false;
            ws->getUserData()->sessionId = sessionId;
            ws->getUserData()->userTopic = sessionTopic;
            
            std::cout << "[WS-DEBUG] Client subscribed to session topic: " << sessionTopic << std::endl;
            LOG_INFO("Client switched to session-specific logs for session: " + sessionId);
            
            // Send confirmation
            nlohmann::json confirmMsg = {
                {"level", "info"},
                {"message", "Subscribed to session logs: " + sessionId},
                {"timestamp", getCurrentTimestamp()}
            };
            std::string confirmJson = confirmMsg.dump();
            ws->send(confirmJson, uWS::OpCode::TEXT);
        }
    }
    // Log other messages for debugging
    else if (!message.empty()) {
        std::cout << "[WS-DEBUG] Non-ping message received: " << message << std::endl;
        LOG_INFO("Received WebSocket message: " + std::string(message));
    }
}

void CrawlLogsWebSocketHandler::onDrain(uWS::WebSocket<false, true, PerSocketData>* ws) {
    // Called when backpressure is relieved - connection can handle more data
    LOG_INFO("WebSocket backpressure relieved for client");
}

void CrawlLogsWebSocketHandler::onClose(uWS::WebSocket<false, true, PerSocketData>* ws, int code, std::string_view message) {
    // Mark socket as closed for lifecycle management
    ws->getUserData()->open = false;
    // uWS automatically unsubscribes on close
    
    std::cout << "[WS-DEBUG] CLIENT DISCONNECTED! Code: " << code << ", Message: '" << message << "'" << std::endl;
    LOG_INFO("Client disconnected from crawl logs WebSocket. Code: " + std::to_string(code) + 
             ", Message: " + std::string(message));
}

void CrawlLogsWebSocketHandler::broadcastLog(const std::string& message, const std::string& level) {
    // Check rate limiting first
    if (shouldThrottleMessage()) {
        std::cout << "[WS-DEBUG] Message throttled due to rate limiting" << std::endl;
        return; // Skip this message due to rate limiting
    }
    
    // Ensure we have an app reference for pub/sub
    if (!globalApp) {
        std::cout << "[WS-DEBUG] No globalApp reference, cannot broadcast" << std::endl;
        return;
    }
    
    // Console log for debugging
    std::cout << "[WS-DEBUG] Broadcasting log message: [" << level << "] " << message << std::endl;
    
    // Create JSON message with timestamp
    nlohmann::json logMsg = {
        {"level", level},
        {"message", message},
        {"timestamp", getCurrentTimestamp()}
    };
    
    std::string jsonString = logMsg.dump();
    
    // Check message size to prevent overwhelming clients
    if (jsonString.size() > 15 * 1024) { // 15KB limit
        LOG_WARNING("WebSocket message too large (" + std::to_string(jsonString.size()) + " bytes), truncating");
        logMsg["message"] = message.substr(0, 1000) + "... [MESSAGE TRUNCATED]";
        jsonString = logMsg.dump();
    }
    
    // Use loop->defer() for thread-safe broadcasting from external threads
    uWS::Loop* loop = serverLoop ? serverLoop : uWS::Loop::get();
    loop->defer([jsonString]() {
        // This lambda runs on the correct uWS thread
        if (globalApp) {
            // Broadcast to admin topic (admin clients see all logs)
            globalApp->publish(CRAWL_LOGS_ADMIN_TOPIC, jsonString, uWS::OpCode::TEXT);
            std::cout << "[WS-DEBUG] Message published to admin topic: " << CRAWL_LOGS_ADMIN_TOPIC << std::endl;
        } else {
            std::cout << "[WS-DEBUG] No globalApp in deferred lambda" << std::endl;
        }
    });
}

void CrawlLogsWebSocketHandler::broadcastToSession(const std::string& sessionId, const std::string& message, const std::string& level) {
    // Check rate limiting first
    if (shouldThrottleMessage()) {
        std::cout << "[WS-DEBUG] Session message throttled due to rate limiting" << std::endl;
        return; // Skip this message due to rate limiting
    }
    
    // Ensure we have an app reference for pub/sub
    if (!globalApp) {
        std::cout << "[WS-DEBUG] No globalApp reference, cannot broadcast to session" << std::endl;
        return;
    }
    
    // Console log for debugging
    std::cout << "[WS-DEBUG] Broadcasting session log message: [" << level << "] " << message << " (Session: " << sessionId << ")" << std::endl;
    
    // Create JSON message with timestamp and session info
    nlohmann::json logMsg = {
        {"level", level},
        {"message", message},
        {"sessionId", sessionId},
        {"timestamp", getCurrentTimestamp()}
    };
    
    std::string jsonString = logMsg.dump();
    
    // Check message size to prevent overwhelming clients
    if (jsonString.size() > 15 * 1024) { // 15KB limit
        LOG_WARNING("WebSocket session message too large (" + std::to_string(jsonString.size()) + " bytes), truncating");
        logMsg["message"] = message.substr(0, 1000) + "... [MESSAGE TRUNCATED]";
        jsonString = logMsg.dump();
    }
    
    // Use loop->defer() for thread-safe broadcasting from external threads
    uWS::Loop* loop = serverLoop ? serverLoop : uWS::Loop::get();
    loop->defer([jsonString, sessionId]() {
        // This lambda runs on the correct uWS thread
        if (globalApp) {
            // Broadcast to both admin topic and session-specific topic
            std::string sessionTopic = std::string(CRAWL_LOGS_SESSION_PREFIX) + sessionId;
            
            // Send to admin (admin sees all sessions)
            globalApp->publish(CRAWL_LOGS_ADMIN_TOPIC, jsonString, uWS::OpCode::TEXT);
            // Send to specific session (user sees only their session)
            globalApp->publish(sessionTopic, jsonString, uWS::OpCode::TEXT);
            
            std::cout << "[WS-DEBUG] Message published to admin and session topic: " << sessionTopic << std::endl;
        } else {
            std::cout << "[WS-DEBUG] No globalApp in session deferred lambda" << std::endl;
        }
    });
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

// Rate limiting function to prevent overwhelming WebSocket clients
bool CrawlLogsWebSocketHandler::shouldThrottleMessage() {
    std::lock_guard<std::mutex> lock(rateLimitMutex);
    
    auto now = std::chrono::steady_clock::now();
    
    // Remove old entries outside the rate limit window
    size_t oldSize = messageTimes.size();
    while (!messageTimes.empty() && 
           now - messageTimes.front() > RATE_LIMIT_WINDOW) {
        messageTimes.pop_front();
    }
    if (oldSize != messageTimes.size()) {
        std::cout << "[WS-DEBUG] Rate limit window cleanup: " << oldSize << " -> " << messageTimes.size() << " messages" << std::endl;
    }
    
    // Check if we've exceeded the rate limit
    if (messageTimes.size() >= MAX_MESSAGES_PER_SECOND) {
        static auto lastThrottleLog = std::chrono::steady_clock::now();
        // Log throttling message only once per second to avoid spam
        if (now - lastThrottleLog > std::chrono::seconds(1)) {
            std::cout << "[WS-DEBUG] ⚠️ RATE LIMITING ACTIVE! Messages: " << messageTimes.size() << "/" << MAX_MESSAGES_PER_SECOND << std::endl;
            LOG_WARNING("WebSocket message rate limiting active - dropping messages");
            lastThrottleLog = now;
        }
        return true;
    }
    
    // Add current time to the queue
    messageTimes.push_back(now);
    std::cout << "[WS-DEBUG] Rate limit check passed: " << messageTimes.size() << "/" << MAX_MESSAGES_PER_SECOND << " messages" << std::endl;
    return false;
}

// Helper function to parse session ID from query parameters
std::string CrawlLogsWebSocketHandler::parseSessionIdFromQuery(const std::string& query) {
    // Simple query parser for sessionId parameter
    // Format: sessionId=crawl_123456789_001 or sessionId=crawl_123456789_001&other=value
    std::string sessionIdPrefix = "sessionId=";
    size_t pos = query.find(sessionIdPrefix);
    
    if (pos != std::string::npos) {
        size_t start = pos + sessionIdPrefix.length();
        size_t end = query.find('&', start);
        
        if (end == std::string::npos) {
            return query.substr(start);
        } else {
            return query.substr(start, end - start);
        }
    }
    
    return ""; // No session ID found
}