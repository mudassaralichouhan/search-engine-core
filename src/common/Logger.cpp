#include "../../include/Logger.h"
#include <iostream>
#include <sstream>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() : logLevel(LogLevel::INFO), logToConsole(true), logToFile(false) {
    // Private constructor implementation
}

Logger::~Logger() {
    close();
}

void Logger::init(LogLevel level, bool enableConsoleLogging, const std::string& logFilePath) {
    std::lock_guard<std::mutex> lock(mutex);
    logLevel = level;
    this->logToConsole = enableConsoleLogging;
    
    if (!logFilePath.empty()) {
        logToFile = true;
        logFile.open(logFilePath, std::ios::out | std::ios::app);
    } else {
        logToFile = false;
    }
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex);
    logLevel = level;
}

bool Logger::isEnabled(LogLevel level) const {
    return level >= logLevel;
}

void Logger::log(LogLevel level, const std::string& message) {
    if (!isEnabled(level)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    std::string levelStr = levelToString(level);
    std::string output = "[" + levelStr + "] " + message;
    
    if (logToConsole) {
        std::cout << output << std::endl;
    }
    
    if (logToFile && logFile.is_open()) {
        logFile << output << std::endl;
        logFile.flush();
    }
}

void Logger::trace(const std::string& message) {
    log(LogLevel::TRACE, message);
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERR, message);
}

void Logger::close() {
    std::lock_guard<std::mutex> lock(mutex);
    if (logFile.is_open()) {
        logFile.close();
    }
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERR: return "ERROR";
        default: return "UNKNOWN";
    }
} 