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
# find_package(CURL REQUIRED)
# find_package(Threads REQUIRED)

# Add include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/public
    ${CMAKE_SOURCE_DIR}/uWebSockets/src
    ${CMAKE_SOURCE_DIR}/uWebSockets/uSockets/src
)

# Add source files
file(GLOB SOURCES "src/*.cpp")

# Add source files for the main executable (excluding crawler sources)
file(GLOB_RECURSE MAIN_SOURCES "src/*.cpp" "src/main.cpp") # Assuming main.cpp is your entry point

# Add Logger implementation
list(APPEND MAIN_SOURCES "src/common/Logger.cpp")

# Remove crawler sources and test sources from MAIN_SOURCES if they were picked up by GLOB_RECURSE
list(FILTER MAIN_SOURCES EXCLUDE REGEX ".*crawler/.*\\.cpp$")
list(FILTER MAIN_SOURCES EXCLUDE REGEX ".*test.*\\.cpp$")

file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")

# Add executable
add_executable(${PROJECT_NAME} ${MAIN_SOURCES} ${HEADERS})

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
