#include "../../include/crawler/CrawlLogger.h"

// Static member definition
CrawlLogger::LogBroadcastFunction CrawlLogger::logBroadcastFunction_ = nullptr;

void CrawlLogger::setLogBroadcastFunction(LogBroadcastFunction func) {
    logBroadcastFunction_ = func;
}

void CrawlLogger::broadcastLog(const std::string& message, const std::string& level) {
    if (logBroadcastFunction_) {
        logBroadcastFunction_(message, level);
    }
    // If no function is set, this is a no-op (safe for tests)
}