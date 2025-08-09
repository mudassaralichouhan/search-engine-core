cmake_minimum_required(VERSION 3.15)
project(Crawler)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Add Gumbo parser - MOVED TO ROOT CMAKE
# include(FetchContent)
# FetchContent_Declare(
#     gumbo
#     GIT_REPOSITORY https://github.com/google/gumbo-parser.git
#     GIT_TAG v0.10.1
# )
# FetchContent_MakeAvailable(gumbo)

# Add source files
set(CRAWLER_SOURCES
    Crawler.cpp
    CrawlerManager.cpp
    CrawlLogger.cpp
    URLFrontier.cpp
    RobotsTxtParser.cpp
    PageFetcher.cpp
    ContentParser.cpp
    FailureClassifier.cpp
    DomainManager.cpp
    CrawlMetrics.cpp
)

# Always add BrowserlessClient.cpp and expose public headers under include/search_engine
list(APPEND CRAWLER_SOURCES BrowserlessClient.cpp)
file(GLOB_RECURSE CRAWLER_PUBLIC_HEADERS
    ${CMAKE_SOURCE_DIR}/include/search_engine/crawler/*.h
)

# Create crawler library
add_library(crawler STATIC ${CRAWLER_SOURCES} ${CRAWLER_PUBLIC_HEADERS})

# Set include directories
target_include_directories(crawler
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CURL_INCLUDE_DIRS}
        ${GUMBO_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(crawler
    PUBLIC
        common
        storage
        ${CURL_LIBRARIES}
        ${GUMBO_LIBRARIES} # Link against system Gumbo found by pkg-config
        Threads::Threads
)

# Set compiler flags (Linux/GCC)
target_compile_options(crawler PRIVATE -Wall -Wextra -Wpedantic)

# Install rules
install(TARGETS crawler
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/search_engine/crawler/
    DESTINATION include/search_engine/crawler
    FILES_MATCHING PATTERN "*.h"
)