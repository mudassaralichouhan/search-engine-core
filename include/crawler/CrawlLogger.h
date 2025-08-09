#pragma once
#include <string>
#include <functional>

// Forward declaration to avoid including WebSocket headers in crawler
class CrawlLogger {
public:
    // Function pointer type for log broadcast (general logs)
    using LogBroadcastFunction = std::function<void(const std::string&, const std::string&)>;
    // Function pointer type for session-specific log broadcast
    using SessionLogBroadcastFunction = std::function<void(const std::string&, const std::string&, const std::string&)>;
    
    // Set the log broadcast functions (called by server during initialization)
    static void setLogBroadcastFunction(LogBroadcastFunction func);
    static void setSessionLogBroadcastFunction(SessionLogBroadcastFunction func);
    
    // Broadcast log message (no-op if function not set)
    static void broadcastLog(const std::string& message, const std::string& level = "info");
    
    // Broadcast session-specific log message
    static void broadcastSessionLog(const std::string& sessionId, const std::string& message, const std::string& level = "info");
    
private:
    static LogBroadcastFunction logBroadcastFunction_;
    static SessionLogBroadcastFunction sessionLogBroadcastFunction_;
};