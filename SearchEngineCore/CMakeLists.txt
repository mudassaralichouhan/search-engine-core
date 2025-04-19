cmake_minimum_required(VERSION 3.10)
project(SearchEngineCore)

# Set vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE "D:/Projects/hatef.ir/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set architecture to x64
set(CMAKE_VS_PLATFORM_NAME "x64")
set(CMAKE_GENERATOR_PLATFORM "x64")

# Set runtime library to MultiThreadedDebugDLL
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDebugDLL")

# MongoDB library paths
set(MONGOC_DRIVER_DIR "C:/mongo-c-driver")
set(MONGOCXX_DRIVER_DIR "C:/mongo-cxx-driver")

# Set environment paths
set(ENV{PATH} "${MONGOCXX_DRIVER_DIR}/bin;${MONGOC_DRIVER_DIR}/bin;$ENV{PATH}")

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Find uWebSockets headers
find_path(UWEBSOCKETS_INCLUDE_DIRS "uwebsockets/App.h"
    PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src
    /usr/local/include
    /usr/include
    ${CMAKE_CURRENT_BINARY_DIR}/deps/install/include
)

if(NOT UWEBSOCKETS_INCLUDE_DIRS)
    message(FATAL_ERROR "uWebSockets headers not found. Please install uWebSockets or set UWEBSOCKETS_INCLUDE_DIRS")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/json/single_include
    D:/Projects/hatef.ir/vcpkg/installed/x64-windows/include
    ${MONGOCXX_DRIVER_DIR}/include/mongocxx/v_noabi
    ${MONGOCXX_DRIVER_DIR}/include/bsoncxx/v_noabi
    ${MONGOC_DRIVER_DIR}/include/libbson-1.0
    ${MONGOC_DRIVER_DIR}/include/libmongoc-1.0
    ${UWEBSOCKETS_INCLUDE_DIRS}
)

# Add source files
set(SOURCES
    src/main.cpp
    src/api.cpp
    src/mongodb.cpp
)

# Add executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link directories
link_directories(
    ${MONGOC_DRIVER_DIR}/lib
    ${MONGOCXX_DRIVER_DIR}/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/lib
    ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows/debug/lib
)

# Create imported targets for MongoDB libraries
add_library(bson-1.0 STATIC IMPORTED)
set_target_properties(bson-1.0 PROPERTIES
    IMPORTED_LOCATION "${MONGOC_DRIVER_DIR}/lib/bson-1.0.lib"
)

add_library(mongoc-1.0 STATIC IMPORTED)
set_target_properties(mongoc-1.0 PROPERTIES
    IMPORTED_LOCATION "${MONGOC_DRIVER_DIR}/lib/mongoc-1.0.lib"
)

add_library(bsoncxx STATIC IMPORTED)
set_target_properties(bsoncxx PROPERTIES
    IMPORTED_LOCATION "${MONGOCXX_DRIVER_DIR}/lib/bsoncxx-v_noabi-rhi-x64-v143-md.lib"
)

add_library(mongocxx STATIC IMPORTED)
set_target_properties(mongocxx PROPERTIES
    IMPORTED_LOCATION "${MONGOCXX_DRIVER_DIR}/lib/mongocxx-v_noabi-rhi-x64-v143-md.lib"
)

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
    nlohmann_json::nlohmann_json
    bson-1.0
    mongoc-1.0
    bsoncxx
    mongocxx
    ws2_32
    iphlpapi
    crypt32
    mswsock
    secur32
    bcrypt
    userenv
    msvcrtd
    msvcprtd
    vcruntimed
    ucrtd
    kernel32
    user32
    gdi32
    winspool
    shell32
    ole32
    oleaut32
    uuid
    comdlg32
    advapi32
)

# Set runtime environment
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_ENVIRONMENT "PATH=${MONGOCXX_DRIVER_DIR}/bin;${MONGOC_DRIVER_DIR}/bin;$(LocalDebuggerEnvironment)"
)

# Add uWebSockets source files
target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src/App.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src/AsyncSocket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src/HttpContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src/HttpParser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src/HttpResponse.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src/HttpRouter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src/Loop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src/PerMessageDeflate.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src/WebSocket.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src/WebSocketContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src/WebSocketExtensions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src/WebSocketHandshake.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src/WebSocketProtocol.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src/WebSocketRouter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uWebSockets/src/WebSocketTransport.cpp
)
