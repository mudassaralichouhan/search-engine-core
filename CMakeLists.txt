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

# Add include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/public
    ${CMAKE_SOURCE_DIR}/uWebSockets/src
    ${CMAKE_SOURCE_DIR}/uWebSockets/uSockets/src
)

# Add source files
file(GLOB SOURCES "src/*.cpp")

# Create executable
add_executable(server ${SOURCES})

# Link libraries
target_link_libraries(server
    PRIVATE
        /usr/local/lib/libmongocxx.so
        /usr/local/lib/libbsoncxx.so
        pthread
        ssl
        crypto
        z
)

# Link uWebSockets object files
file(GLOB UWS_OBJECTS "${CMAKE_SOURCE_DIR}/uWebSockets/uSockets/*.o")
target_link_libraries(server
    PRIVATE
        ${UWS_OBJECTS}
)
