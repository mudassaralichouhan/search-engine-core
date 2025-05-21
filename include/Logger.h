#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <sstream>

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARNING = 3,
    ERR = 4,     // Changed from ERROR to ERR to avoid conflict with system macros
    NONE = 5     // No logging
};

class Logger {
public:
    static Logger& getInstance();
    
    // Initialize the logger
    void init(LogLevel level = LogLevel::INFO, bool enableConsoleLogging = true, const std::string& logFilePath = "");
    
    // Set log level
    void setLogLevel(LogLevel level);
    
    // Check if a given log level is enabled
    bool isEnabled(LogLevel level) const;
    
    // Log a message with a specific level
    void log(LogLevel level, const std::string& message);
    
    // Helper methods for different log levels
    void trace(const std::string& message);
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    
    // Close log file if open
    void close();
    
    // Destructor
    ~Logger();

    // Get current log level
    LogLevel getLogLevel() const {
        return logLevel;
    }

private:
    // Private constructor for singleton
    Logger();
    
    // Disable copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // Convert log level to string
    std::string levelToString(LogLevel level) const;
    
    LogLevel logLevel;
    bool logToConsole;
    bool logToFile;
    std::ofstream logFile;
    std::mutex mutex;
};

// Convenience macros for logging
#define LOG_TRACE(message) Logger::getInstance().trace(message)
#define LOG_DEBUG(message) Logger::getInstance().debug(message)
#define LOG_INFO(message) Logger::getInstance().info(message)
#define LOG_WARNING(message) Logger::getInstance().warning(message)
#define LOG_ERROR(message) Logger::getInstance().error(message)

// Stream-style logging macros
#define LOG_TRACE_STREAM(message) { if (Logger::getInstance().isEnabled(LogLevel::TRACE)) { std::stringstream ss; ss << message; Logger::getInstance().trace(ss.str()); } }
#define LOG_DEBUG_STREAM(message) { if (Logger::getInstance().isEnabled(LogLevel::DEBUG)) { std::stringstream ss; ss << message; Logger::getInstance().debug(ss.str()); } }
#define LOG_INFO_STREAM(message) { if (Logger::getInstance().isEnabled(LogLevel::INFO)) { std::stringstream ss; ss << message; Logger::getInstance().info(ss.str()); } }
#define LOG_WARNING_STREAM(message) { if (Logger::getInstance().isEnabled(LogLevel::WARNING)) { std::stringstream ss; ss << message; Logger::getInstance().warning(ss.str()); } }
#define LOG_ERROR_STREAM(message) { if (Logger::getInstance().isEnabled(LogLevel::ERR)) { std::stringstream ss; ss << message; Logger::getInstance().error(ss.str()); } } 