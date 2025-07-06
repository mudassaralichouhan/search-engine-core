# Webserver tests
cmake_minimum_required(VERSION 3.10)

# Find Catch2
find_package(Catch2 3 REQUIRED)

# Add test executable
add_executable(SearchEndpointTest 
    SearchEndpointTest.cpp
    ${CMAKE_SOURCE_DIR}/src/search_api.cpp
    ${CMAKE_SOURCE_DIR}/src/api.cpp
    ${CMAKE_SOURCE_DIR}/src/infrastructure.cpp
    ${CMAKE_SOURCE_DIR}/src/utils.cpp
    ${CMAKE_SOURCE_DIR}/src/mongodb.cpp
)

# Include directories
target_include_directories(SearchEndpointTest PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/search_core
    ${CMAKE_SOURCE_DIR}/include/search_engine/storage
    /usr/local/include/uwebsockets
    /usr/local/include/usockets
)

# Link libraries
target_link_libraries(SearchEndpointTest PRIVATE
    Catch2::Catch2
    common
    search_core
    storage
    /usr/local/lib/libuSockets.a
    /usr/local/lib/libmongocxx.so
    /usr/local/lib/libbsoncxx.so
    pthread
    z
    ssl
    crypto
)

# If Redis is available, link Redis libraries
if(REDIS_AVAILABLE)
    target_link_libraries(SearchEndpointTest PRIVATE ${REDIS_LIBRARIES})
    target_compile_definitions(SearchEndpointTest PRIVATE REDIS_AVAILABLE)
endif()

# Add test to CTest
include(CTest)
include(Catch)
catch_discover_tests(SearchEndpointTest)

# Add custom test target
add_test(NAME SearchEndpointTest COMMAND SearchEndpointTest) 