#pragma once
#include <string>
#include <functional>

// Forward declaration to avoid including WebSocket headers in crawler
class CrawlLogger {
public:
    // Function pointer type for log broadcast
    using LogBroadcastFunction = std::function<void(const std::string&, const std::string&)>;
    
    // Set the log broadcast function (called by server during initialization)
    static void setLogBroadcastFunction(LogBroadcastFunction func);
    
    // Broadcast log message (no-op if function not set)
    static void broadcastLog(const std::string& message, const std::string& level = "info");
    
private:
    static LogBroadcastFunction logBroadcastFunction_;
};