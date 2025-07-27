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
    URLFrontier.cpp
    RobotsTxtParser.cpp
    PageFetcher.cpp
    ContentParser.cpp
)

# Always add BrowserlessClient.cpp and header
list(APPEND CRAWLER_SOURCES BrowserlessClient.cpp)
list(APPEND CRAWLER_HEADERS ../../include/BrowserlessClient.h)

# Create crawler library
add_library(crawler STATIC ${CRAWLER_SOURCES} ${CRAWLER_HEADERS})

# Set include directories
target_include_directories(crawler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CURL_INCLUDE_DIRS}
        ${GUMBO_INCLUDE_DIRS} # Add system Gumbo include directories
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

install(FILES ${CRAWLER_HEADERS}
    DESTINATION include/crawler
) 