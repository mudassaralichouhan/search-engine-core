# Common Tests CMakeLists.txt

# Find Catch2 package
find_package(Catch2 3 REQUIRED)

# Test executable for JS Minifier file processing
add_executable(test_js_minifier_file TestJsMinifierFile.cpp)

# Link libraries
target_link_libraries(test_js_minifier_file
    PRIVATE
        Catch2::Catch2WithMain
        common
        ${CURL_LIBRARIES}
        CURL::libcurl
)

# Include directories
target_include_directories(test_js_minifier_file PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

# Register tests with CTest
include(CTest)
include(Catch)

# Add test to CTest
add_test(NAME js_minifier_file_test COMMAND test_js_minifier_file)
set_tests_properties(js_minifier_file_test PROPERTIES
    TIMEOUT 60
    LABELS "common;js_minifier;file_processing"
)

# Auto-discover test cases
catch_discover_tests(test_js_minifier_file)

message(STATUS "Common tests configured with JS Minifier file processing test")
