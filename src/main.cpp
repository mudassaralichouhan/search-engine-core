#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <filesystem>
#include <uwebsockets/App.h>
#include "../include/routing/RouteRegistry.h"
#include "../include/Logger.h"

// Include all controllers to trigger their static initialization
#include "controllers/HomeController.h"
#include "controllers/SearchController.h"
#include "controllers/StaticFileController.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <atomic>
#include <unordered_set>
#include "websocket/WebSocketRegistry.h"
#include "websocket/DateTimeWebSocketHandler.h"
#include "websocket/CrawlLogsWebSocketHandler.h"
#include "../include/crawler/CrawlLogger.h"

using namespace std;



// Helper function to get current timestamp
std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();
    return ss.str();
}

// Request tracing middleware
void traceRequest(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    std::string_view method = req->getMethod();
    std::string_view path = req->getUrl();
    std::string_view query = req->getQuery();
    
    std::string logMessage = "[" + getCurrentTimestamp() + "] " + std::string(method) + " " + std::string(path);
    
    if (!query.empty()) {
        logMessage += "?" + std::string(query);
    }
    
    // Log headers
    logMessage += "\nHeaders:";
    // Note: uWebSockets doesn't provide direct header iteration
    // We can log specific headers we're interested in
    logMessage += "\n  User-Agent: " + std::string(req->getHeader("user-agent"));
    logMessage += "\n  Accept: " + std::string(req->getHeader("accept"));
    logMessage += "\n  Content-Type: " + std::string(req->getHeader("content-type"));
    
    LOG_INFO(logMessage);
}

int main() {
    // Initialize logger
    Logger::getInstance().init(LogLevel::INFO, true, "server.log");
    
    // Log registered routes
    LOG_INFO("=== Registered Routes ===");
    for (const auto& route : routing::RouteRegistry::getInstance().getRoutes()) {
        LOG_INFO(routing::methodToString(route.method) + " " + route.path + 
                 " -> " + route.controllerName + "::" + route.actionName);
    }
    LOG_INFO("========================");

    // Get port from environment variable or use default
    const char* port_env = std::getenv("PORT");
    int port = port_env ? std::stoi(port_env) : 3000;
    LOG_INFO("Using port: " + std::to_string(port));

    // Create app and apply all registered routes
    auto app = uWS::App();

    // WebSocket registry and handler injection
    WebSocketRegistry wsRegistry;
    wsRegistry.addHandler(std::make_shared<DateTimeWebSocketHandler>());
    // Create and register crawl logs WebSocket handler
    wsRegistry.addHandler(std::make_shared<CrawlLogsWebSocketHandler>());
    wsRegistry.registerAll(app);
    
    // Connect CrawlLogger to WebSocket handler for real-time logging
    CrawlLogger::setLogBroadcastFunction([](const std::string& message, const std::string& level) {
        CrawlLogsWebSocketHandler::broadcastLog(message, level);
    });
    
    // Add request tracing middleware wrapper
    routing::RouteRegistry::getInstance().applyRoutes(app);
    
    // Start the server
    app.listen(port, [port](auto* listen_socket) {
        
        if (listen_socket) {
            LOG_INFO("Server listening on port " + std::to_string(port));
            LOG_INFO("Access the search engine at: http://localhost:" + std::to_string(port) + "/test");
            LOG_INFO("Coming soon page at: http://localhost:" + std::to_string(port) + "/");
        }
        else {
            LOG_ERROR("Failed to listen on port " + std::to_string(port));
        }
    }).run();

    return 0;
}

