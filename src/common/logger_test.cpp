#include "../../include/Logger.h"
#include <iostream>

int main() {
    // Initialize logger with DEBUG level
    Logger::getInstance().init(LogLevel::DEBUG, true, "log_test.log");
    
    // Test different log levels
    LOG_TRACE("This is a TRACE message");
    LOG_DEBUG("This is a DEBUG message");
    LOG_INFO("This is an INFO message");
    LOG_WARNING("This is a WARNING message");
    LOG_ERROR("This is an ERROR message");
    
    // Test stream-style logging
    LOG_INFO_STREAM("Stream logging: " << 42 << " is the answer");
    
    std::cout << "Logger test completed. Check log_test.log for output." << std::endl;
    
    return 0;
} 