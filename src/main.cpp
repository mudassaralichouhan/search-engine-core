#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <filesystem>
#include <algorithm>
#include <uwebsockets/App.h>
#include "../include/routing/RouteRegistry.h"
#include "../include/Logger.h"

// Include all controllers to trigger their static initialization
// NOTE: This still triggers static initialization, but controllers like DomainController
// now use lazy initialization to avoid the static initialization order fiasco.
// TODO: Consider implementing lazy route registration to completely eliminate
// static initialization issues.
#include "controllers/HomeController.h"
#include "controllers/SearchController.h"
#include "controllers/StaticFileController.h"
#include "controllers/DomainController.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <atomic>
#include <unordered_set>
#include "websocket/WebSocketRegistry.h"
#include "websocket/DateTimeWebSocketHandler.h"
#include "websocket/CrawlLogsWebSocketHandler.h"
#include <iostream>
#include "../include/crawler/CrawlLogger.h"
#include <csignal>
#include <execinfo.h>

using namespace std;
// Global flag for graceful shutdown
std::atomic<bool> g_shutdown_requested{false};

// Graceful shutdown function
void requestShutdown() {
    g_shutdown_requested = true;
    LOG_ERROR("Shutdown requested - attempting graceful termination");
}

// Crash handler to log a backtrace on segfaults
void installCrashHandler() {
    auto crash_handler = [](int sig) {
        // Log the crash immediately
        LOG_ERROR("FATAL SIGNAL RECEIVED: " + std::to_string(sig));

        // Get backtrace
        void* array[64];
        size_t size = backtrace(array, 64);
        char** messages = backtrace_symbols(array, size);

        std::cerr << "[FATAL] Signal " << sig << " received. Backtrace (" << size << "):\n";
        if (messages) {
            for (size_t i = 0; i < size; ++i) {
                std::cerr << messages[i] << "\n";
                LOG_ERROR("Backtrace[" + std::to_string(i) + "]: " + std::string(messages[i]));
            }
            free(messages);
        }
        std::cerr.flush();

        // For critical signals, we need immediate exit to prevent further damage
        // But for less critical signals, we could potentially allow cleanup
        if (sig == SIGSEGV || sig == SIGABRT) {
            // Critical signals - immediate exit
            LOG_ERROR("Critical signal received - immediate exit required");
            _exit(128 + sig);
        } else {
            // Other signals - attempt graceful shutdown
            LOG_WARNING("Non-critical signal received - attempting graceful shutdown");
            requestShutdown();
            std::exit(128 + sig);
        }
    };

    // Install handlers for common crash signals
    std::signal(SIGSEGV, crash_handler);  // Segmentation fault
    std::signal(SIGABRT, crash_handler);  // Abort signal
    std::signal(SIGILL, crash_handler);   // Illegal instruction
    std::signal(SIGFPE, crash_handler);   // Floating point exception
    std::signal(SIGTERM, crash_handler);  // Termination request
    std::signal(SIGINT, crash_handler);   // Interrupt (Ctrl+C)

    LOG_INFO("Crash handler installed for signals: SEGV, ABRT, ILL, FPE, TERM, INT");
}



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


// RAII WebSocket Manager class
class WebSocketManager {
public:
    WebSocketManager() {
        LOG_DEBUG("WebSocketManager: Initializing WebSocket registry...");
        registry_ = std::make_unique<WebSocketRegistry>();
    }

    ~WebSocketManager() {
        LOG_DEBUG("WebSocketManager: Cleaning up WebSocket registry...");
        // Registry will be automatically cleaned up by unique_ptr
        LOG_INFO("WebSocket registry cleaned up successfully");
    }

    void addHandler(std::shared_ptr<WebSocketHandler> handler) {
        if (registry_ && handler) {
            registry_->addHandler(handler);
            LOG_DEBUG("WebSocketManager: Added handler successfully");
        }
    }

    void registerAll(uWS::App& app) {
        if (registry_) {
            registry_->registerAll(app);
            LOG_DEBUG("WebSocketManager: All handlers registered with uWS app");
        }
    }

    WebSocketRegistry* getRegistry() {
        return registry_.get();
    }

private:
    std::unique_ptr<WebSocketRegistry> registry_;
};

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
    LOG_DEBUG("============== SEARCH ENGINE STARTING ==============");
    installCrashHandler();

    // Initialize logger with configurable log level
    LOG_DEBUG("Initializing logger...");

    // Get log level from environment variable or use default
    LogLevel logLevel = LogLevel::INFO; // Default
    const char* logLevel_env = std::getenv("LOG_LEVEL");
    if (logLevel_env) {
        std::string logLevelStr = logLevel_env;
        std::transform(logLevelStr.begin(), logLevelStr.end(), logLevelStr.begin(), ::tolower);
        if (logLevelStr == "trace") logLevel = LogLevel::TRACE;
        else if (logLevelStr == "debug") logLevel = LogLevel::DEBUG;
        else if (logLevelStr == "info") logLevel = LogLevel::INFO;
        else if (logLevelStr == "warning") logLevel = LogLevel::WARNING;
        else if (logLevelStr == "error") logLevel = LogLevel::ERR;
        else if (logLevelStr == "none") logLevel = LogLevel::NONE;
        else {
            LOG_WARNING("Invalid LOG_LEVEL environment variable: " + std::string(logLevel_env) + ". Using default INFO level.");
        }
    }

    Logger::getInstance().init(logLevel, true, "server.log");
    LOG_DEBUG("Logger initialized successfully with level: " + std::to_string(static_cast<int>(logLevel)));

    // Log registered routes
    LOG_DEBUG("Logging registered routes...");
    LOG_INFO("=== Registered Routes ===");
    for (const auto& route : routing::RouteRegistry::getInstance().getRoutes()) {
        LOG_INFO(routing::methodToString(route.method) + " " + route.path + 
                 " -> " + route.controllerName + "::" + route.actionName);
    }
    LOG_INFO("========================");
    LOG_DEBUG("All routes logged");

    // Get port from environment variable or use default
    const char* port_env = std::getenv("PORT");
    int port = 3000; // Default port
    try {
        if (port_env) {
            port = std::stoi(port_env);
            if (port < 1 || port > 65535) {
                LOG_WARNING("Invalid port number in PORT environment variable: " + std::string(port_env) + ". Using default port 3000.");
                port = 3000;
            }
        }
    } catch (const std::exception& e) {
        LOG_WARNING("Invalid PORT environment variable: " + std::string(port_env ? port_env : "null") + ". Using default port 3000. Error: " + e.what());
        port = 3000;
    }
    LOG_DEBUG("Using port: " + std::to_string(port));
    LOG_INFO("Using port: " + std::to_string(port));

    // Create app and apply all registered routes
    LOG_DEBUG("Creating uWebSockets app...");
    auto app = uWS::App();
    LOG_DEBUG("uWebSockets app created successfully");

    // WebSocket manager with RAII - automatically handles cleanup
    WebSocketManager wsManager;
    wsManager.addHandler(std::make_shared<DateTimeWebSocketHandler>());
    wsManager.addHandler(std::make_shared<CrawlLogsWebSocketHandler>());
    wsManager.registerAll(app);

    // Connect CrawlLogger to WebSocket handler for real-time logging
    LOG_DEBUG("Setting up WebSocket broadcast functions...");

    // General broadcast function (for admin and legacy support)
    CrawlLogger::setLogBroadcastFunction([](const std::string& message, const std::string& level) {
        LOG_DEBUG("Lambda called for WebSocket broadcast: [" + level + "] " + message);
        CrawlLogsWebSocketHandler::broadcastLog(message, level);
    });

    // Session-specific broadcast function
    CrawlLogger::setSessionLogBroadcastFunction([](const std::string& sessionId, const std::string& message, const std::string& level) {
        LOG_DEBUG("Lambda called for session WebSocket broadcast: [" + level + "] " + message + " (Session: " + sessionId + ")");
        CrawlLogsWebSocketHandler::broadcastToSession(sessionId, message, level);
    });

    LOG_DEBUG("WebSocket broadcast functions set successfully");

    // Add request tracing middleware wrapper
    LOG_DEBUG("Applying routes to app...");
    routing::RouteRegistry::getInstance().applyRoutes(app);
    LOG_DEBUG("Routes applied successfully");

    // Start the server
    LOG_DEBUG("Starting server on port " + std::to_string(port) + "...");
    app.listen(port, [port](auto* listen_socket) {

        if (listen_socket) {
            LOG_DEBUG("✅ SERVER STARTED SUCCESSFULLY! Port: " + std::to_string(port));
            LOG_DEBUG("✅ WebSocket endpoint: ws://localhost:" + std::to_string(port) + "/crawl-logs");
            LOG_DEBUG("✅ Crawl tester page: http://localhost:" + std::to_string(port) + "/crawl-tester.html");
            LOG_INFO("Server listening on port " + std::to_string(port));
            LOG_INFO("Access the search engine at: http://localhost:" + std::to_string(port) + "/test");
            LOG_INFO("Coming soon page at: http://localhost:" + std::to_string(port) + "/");
        }
        else {
            LOG_DEBUG("❌ FAILED TO START SERVER on port " + std::to_string(port));
            LOG_ERROR("Failed to listen on port " + std::to_string(port));
        }
    }).run();

    LOG_DEBUG("============== SEARCH ENGINE STOPPED ==============");

    return 0;
}

