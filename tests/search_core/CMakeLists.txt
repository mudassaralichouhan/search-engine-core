cmake_minimum_required(VERSION 3.24)

# Find Catch2 package
find_package(Catch2 3 REQUIRED)

# Helper function to create test executables
function(create_search_test test_name test_file)
    add_executable(${test_name} ${test_file})
    
    target_link_libraries(${test_name} 
        PRIVATE 
            search_core
            Catch2::Catch2WithMain
    )
    
    target_compile_features(${test_name} PRIVATE cxx_std_20)
    
    # Register the test with CTest
    catch_discover_tests(${test_name}
        TEST_PREFIX "search_core::"
        PROPERTIES 
            LABELS "search_core"
    )
endfunction()

# Create individual test executables
create_search_test(test_search_client TestSearchClient.cpp)
create_search_test(test_query_parser TestQueryParser.cpp)
create_search_test(test_scorer TestScorer.cpp)
create_search_test(test_exact_search_e2e TestExactSearchE2E.cpp)

# Note: Test properties are set through catch_discover_tests above
# Integration and performance test labels are handled by Catch2 tags

# Include Catch2 test discovery
include(Catch) 