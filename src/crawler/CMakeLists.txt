cmake_minimum_required(VERSION 3.15)
project(Crawler)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Add Gumbo parser
include(FetchContent)
FetchContent_Declare(
    gumbo
    GIT_REPOSITORY https://github.com/google/gumbo-parser.git
    GIT_TAG v0.10.1
)
FetchContent_MakeAvailable(gumbo)

# Add source files
set(CRAWLER_SOURCES
    Crawler.cpp
    URLFrontier.cpp
    RobotsTxtParser.cpp
    PageFetcher.cpp
    ContentParser.cpp
)

# Add header files
set(CRAWLER_HEADERS
    Crawler.h
    URLFrontier.h
    RobotsTxtParser.h
    PageFetcher.h
    ContentParser.h
    models/CrawlResult.h
    models/CrawlConfig.h
)

# Create crawler library
add_library(crawler STATIC ${CRAWLER_SOURCES} ${CRAWLER_HEADERS})

# Set include directories
target_include_directories(crawler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CURL_INCLUDE_DIRS}
        ${gumbo_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(crawler
    PUBLIC
        ${CURL_LIBRARIES}
        gumbo
        Threads::Threads
)

# Set compiler flags
if(MSVC)
    target_compile_options(crawler PRIVATE /W4)
else()
    target_compile_options(crawler PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install rules
install(TARGETS crawler
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES ${CRAWLER_HEADERS}
    DESTINATION include/crawler
) 