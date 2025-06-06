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
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GUMBO REQUIRED gumbo)
# find_package(Threads REQUIRED)

# Redis detection - check for hiredis and redis-plus-plus
find_path(HIREDIS_HEADER hiredis)
find_library(HIREDIS_LIB hiredis)
find_path(REDIS_PLUS_PLUS_HEADER sw)
find_library(REDIS_PLUS_PLUS_LIB redis++)

if(HIREDIS_HEADER AND HIREDIS_LIB AND REDIS_PLUS_PLUS_HEADER AND REDIS_PLUS_PLUS_LIB)
    message(STATUS "Redis and redis-plus-plus found - RedisSearchStorage will be available")
    message(STATUS "  hiredis include: ${HIREDIS_HEADER}")
    message(STATUS "  hiredis library: ${HIREDIS_LIB}")
    message(STATUS "  redis++ include: ${REDIS_PLUS_PLUS_HEADER}")
    message(STATUS "  redis++ library: ${REDIS_PLUS_PLUS_LIB}")
    add_definitions(-DREDIS_AVAILABLE)
    set(REDIS_AVAILABLE ON)
else()
    message(STATUS "Redis libraries not found - RedisSearchStorage will not be available")
    set(REDIS_AVAILABLE OFF)
endif()

# Add include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/public
    ${CMAKE_SOURCE_DIR}/uWebSockets/src
    ${CMAKE_SOURCE_DIR}/uWebSockets/uSockets/src
    ${GUMBO_INCLUDE_DIRS}
)

# Add Redis include directories if available
if(REDIS_AVAILABLE)
    include_directories(${HIREDIS_HEADER} ${REDIS_PLUS_PLUS_HEADER})
endif()

# Add source files recursively to include subdirectories like src/common/
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Remove test sources from SOURCES if they were picked up by GLOB_RECURSE
list(FILTER SOURCES EXCLUDE REGEX ".*test.*\\.cpp$")

file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")

# Create executable
add_executable(server ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(server
    PRIVATE
        /usr/local/lib/libmongocxx.so
        /usr/local/lib/libbsoncxx.so
        pthread
        ssl
        crypto
        z
        ${CURL_LIBRARIES}
        CURL::libcurl
        ${GUMBO_LIBRARIES}
)

# Link Redis libraries if available
if(REDIS_AVAILABLE)
    target_link_libraries(server
        PRIVATE
            ${REDIS_PLUS_PLUS_LIB}
            ${HIREDIS_LIB}
    )
endif()

# Link uWebSockets object files
file(GLOB UWS_OBJECTS "${CMAKE_SOURCE_DIR}/uWebSockets/uSockets/*.o")
target_link_libraries(server
    PRIVATE
        ${UWS_OBJECTS}
)
