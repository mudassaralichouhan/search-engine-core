cmake_minimum_required(VERSION 3.10)
project(search_engine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add MongoDB specific paths
include_directories(/usr/local/include/mongocxx/v_noabi /usr/local/include/bsoncxx/v_noabi)
list(APPEND CMAKE_PREFIX_PATH "/usr/local/lib/cmake/mongocxx-4.0.0")
list(APPEND CMAKE_PREFIX_PATH "/usr/local/lib/cmake/bsoncxx-4.0.0")

# Find required packages
find_package(mongocxx REQUIRED CONFIG)
find_package(bsoncxx REQUIRED CONFIG)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GUMBO REQUIRED gumbo)
find_package(Threads REQUIRED)

# Redis and redis-plus-plus detection
find_path(HIREDIS_INCLUDE_DIR NAMES hiredis/hiredis.h PATHS /usr/include /usr/local/include)
find_library(HIREDIS_LIBRARY NAMES hiredis PATHS /usr/lib /usr/local/lib)
find_path(REDIS_PLUS_PLUS_INCLUDE_DIR NAMES sw/redis++/redis++.h PATHS /usr/include /usr/local/include)
find_library(REDIS_PLUS_PLUS_LIBRARY NAMES redis++ PATHS /usr/lib /usr/local/lib)

if(HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARY AND REDIS_PLUS_PLUS_INCLUDE_DIR AND REDIS_PLUS_PLUS_LIBRARY)
    set(REDIS_AVAILABLE TRUE)
    set(REDIS_LIBRARIES ${HIREDIS_LIBRARY} ${REDIS_PLUS_PLUS_LIBRARY})
    set(REDIS_INCLUDE_DIRS ${HIREDIS_INCLUDE_DIR} ${REDIS_PLUS_PLUS_INCLUDE_DIR})
    message(STATUS "Redis and redis-plus-plus found - RedisSearchStorage will be available")
    message(STATUS "  hiredis: ${HIREDIS_LIBRARY}")
    message(STATUS "  redis++: ${REDIS_PLUS_PLUS_LIBRARY}")
else()
    set(REDIS_AVAILABLE FALSE)
    message(STATUS "Redis dependencies not found - RedisSearchStorage will be excluded")
    if(NOT HIREDIS_INCLUDE_DIR OR NOT HIREDIS_LIBRARY)
        message(STATUS "  Missing hiredis")
    endif()
    if(NOT REDIS_PLUS_PLUS_INCLUDE_DIR OR NOT REDIS_PLUS_PLUS_LIBRARY)
        message(STATUS "  Missing redis-plus-plus")
    endif()
endif()

# Add include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/public
    /usr/local/include/uwebsockets
    /usr/local/include/usockets
    ${GUMBO_INCLUDE_DIRS}
)

# Add Redis include directories if available
if(REDIS_AVAILABLE)
    include_directories(${REDIS_INCLUDE_DIRS})
endif()

# Create a common library for shared code
add_library(common STATIC
    src/common/Logger.cpp
)
target_include_directories(common 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Install common library
install(TARGETS common
    EXPORT CommonTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Add subdirectories for libraries
add_subdirectory(src/crawler)
add_subdirectory(src/storage)
add_subdirectory(src/scoring)
add_subdirectory(src/search_core)

# Add source files recursively to include subdirectories like src/common/
# file(GLOB_RECURSE SOURCES "src/*.cpp")

# # Remove test sources from SOURCES if they were picked up by GLOB_RECURSE
# list(FILTER SOURCES EXCLUDE REGEX ".*test.*\\.cpp$")

# file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")

file(GLOB_RECURSE MAIN_SOURCES "src/*.cpp" "src/main.cpp")

# Remove crawler sources, storage sources, test sources, and common sources from MAIN_SOURCES
list(FILTER MAIN_SOURCES EXCLUDE REGEX ".*crawler/.*\\.cpp$")
list(FILTER MAIN_SOURCES EXCLUDE REGEX ".*storage/.*\\.cpp$")
list(FILTER MAIN_SOURCES EXCLUDE REGEX ".*test.*\\.cpp$")
list(FILTER MAIN_SOURCES EXCLUDE REGEX ".*common/.*\\.cpp$")

file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")

# Create executable
add_executable(server ${MAIN_SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(server
    PRIVATE
        common
        storage
        crawler
        search_core
        scoring
        /usr/local/lib/libmongocxx.so
        /usr/local/lib/libbsoncxx.so
        OpenSSL::SSL
        OpenSSL::Crypto
        pthread
        z
        ${CURL_LIBRARIES}
        CURL::libcurl
        ${GUMBO_LIBRARIES}
        Threads::Threads
)

# Link Redis libraries if available
if(REDIS_AVAILABLE)
    target_link_libraries(server PRIVATE ${REDIS_LIBRARIES})
    target_compile_definitions(server PRIVATE REDIS_AVAILABLE)
endif()

# Link uSockets library
target_link_libraries(server
    PRIVATE
        /usr/local/lib/libuSockets.a
)

# Enable testing and add tests
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
