#include "../../include/crawler/CrawlLogger.h"
#include <iostream>

// Static member definitions
CrawlLogger::LogBroadcastFunction CrawlLogger::logBroadcastFunction_ = nullptr;
CrawlLogger::SessionLogBroadcastFunction CrawlLogger::sessionLogBroadcastFunction_ = nullptr;

void CrawlLogger::setLogBroadcastFunction(LogBroadcastFunction func) {
    logBroadcastFunction_ = func;
}

void CrawlLogger::setSessionLogBroadcastFunction(SessionLogBroadcastFunction func) {
    sessionLogBroadcastFunction_ = func;
}

void CrawlLogger::broadcastLog(const std::string& message, const std::string& level) {
    std::cout << "[CRAWL-DEBUG] CrawlLogger::broadcastLog called with: [" << level << "] " << message << std::endl;
    
    if (logBroadcastFunction_) {
        std::cout << "[CRAWL-DEBUG] Calling WebSocket broadcast function..." << std::endl;
        logBroadcastFunction_(message, level);
        std::cout << "[CRAWL-DEBUG] WebSocket broadcast function completed" << std::endl;
    } else {
        std::cout << "[CRAWL-DEBUG] No WebSocket broadcast function set - message not sent" << std::endl;
    }
    // If no function is set, this is a no-op (safe for tests)
}

void CrawlLogger::broadcastSessionLog(const std::string& sessionId, const std::string& message, const std::string& level) {
    std::cout << "[CRAWL-DEBUG] CrawlLogger::broadcastSessionLog called with: [" << level << "] " << message << " (Session: " << sessionId << ")" << std::endl;
    
    if (sessionLogBroadcastFunction_) {
        std::cout << "[CRAWL-DEBUG] Calling session WebSocket broadcast function..." << std::endl;
        sessionLogBroadcastFunction_(sessionId, message, level);
        std::cout << "[CRAWL-DEBUG] Session WebSocket broadcast function completed" << std::endl;
    } else {
        std::cout << "[CRAWL-DEBUG] No session WebSocket broadcast function set - using general broadcast" << std::endl;
        // Fallback to general broadcast if session function not available
        broadcastLog(message + " (Session: " + sessionId + ")", level);
    }
}