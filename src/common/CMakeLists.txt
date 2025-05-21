cmake_minimum_required(VERSION 3.12)
project(common)

# Add logger_test executable
add_executable(logger_test logger_test.cpp)

# Link with Logger.cpp
target_sources(logger_test PRIVATE Logger.cpp)

# Include directories
target_include_directories(logger_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)

# Set C++ standard
set_target_properties(logger_test PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
) 