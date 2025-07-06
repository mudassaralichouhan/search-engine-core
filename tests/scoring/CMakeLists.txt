# Find Catch2
find_package(Catch2 3 REQUIRED)

# Test executable
add_executable(test_search_scorer test_search_scorer.cpp)

# Link libraries
target_link_libraries(test_search_scorer
    PRIVATE
        Catch2::Catch2WithMain
        scoring
        storage
)

# Add Redis libraries if available
if(REDIS_AVAILABLE)
    target_link_libraries(test_search_scorer PRIVATE ${REDIS_LIBRARIES})
    target_compile_definitions(test_search_scorer PRIVATE REDIS_AVAILABLE)
endif()

# Register tests with CTest
include(CTest)
include(Catch)
catch_discover_tests(test_search_scorer) 