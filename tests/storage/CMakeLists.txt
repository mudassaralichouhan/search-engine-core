# Storage Tests CMakeLists.txt

# Find required packages for tests
find_package(Catch2 3 REQUIRED)

# Make sure storage library is available
if(NOT TARGET storage)
    message(FATAL_ERROR "Storage library target not found. Make sure src/storage is built first.")
endif()

# MongoDB paths (should match main CMakeLists.txt)
set(MONGOC_DRIVER_DIR "C:/mongo-c-driver")
set(MONGOCXX_DRIVER_DIR "C:/mongo-cxx-driver")

# Test source files - conditionally include Redis-dependent tests
set(TEST_SOURCES
    test_mongodb_storage.cpp
)

# Add Redis-dependent tests only if Redis is available
if(DEFINED REDIS_AVAILABLE AND REDIS_AVAILABLE)
    list(APPEND TEST_SOURCES 
        test_redis_search_storage.cpp
        test_content_storage.cpp
    )
endif()

# Create test executables
foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    
    # Link with storage library and test framework
    target_link_libraries(${TEST_NAME} PRIVATE
        Catch2::Catch2WithMain
        storage
        SearchEngine::Storage
    )
    
    # Include directories
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    )
    
    # Set runtime environment for MongoDB DLLs
    set_target_properties(${TEST_NAME} PROPERTIES
        VS_DEBUGGER_ENVIRONMENT "PATH=${MONGOCXX_DRIVER_DIR}/bin;${MONGOC_DRIVER_DIR}/bin;$(LocalDebuggerEnvironment)"
    )
    
    # Add test to CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    
    # Set test properties with environment
    set_tests_properties(${TEST_NAME} PROPERTIES
        TIMEOUT 300
        LABELS "storage"
        ENVIRONMENT "PATH=${MONGOCXX_DRIVER_DIR}/bin;${MONGOC_DRIVER_DIR}/bin;$ENV{PATH}"
    )
endforeach()

# Create a combined storage test executable
add_executable(storage_tests ${TEST_SOURCES})

target_link_libraries(storage_tests PRIVATE
    Catch2::Catch2WithMain
    storage
    SearchEngine::Storage
)

target_include_directories(storage_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

# Set runtime environment for MongoDB DLLs
set_target_properties(storage_tests PROPERTIES
    VS_DEBUGGER_ENVIRONMENT "PATH=${MONGOCXX_DRIVER_DIR}/bin;${MONGOC_DRIVER_DIR}/bin;$(LocalDebuggerEnvironment)"
)

# Add combined test to CTest with proper environment
add_test(NAME storage_tests COMMAND storage_tests)
set_tests_properties(storage_tests PROPERTIES
    TIMEOUT 600
    LABELS "storage;integration"
    ENVIRONMENT "PATH=${MONGOCXX_DRIVER_DIR}/bin;${MONGOC_DRIVER_DIR}/bin;$ENV{PATH}"
)

# Note: Disabled catch_discover_tests to avoid MongoDB DLL dependency during CMake configuration
# The tests can still be run manually or via CTest with the proper environment
message(STATUS "Storage tests configured. Run tests manually with proper MongoDB DLL path or use CTest.") 