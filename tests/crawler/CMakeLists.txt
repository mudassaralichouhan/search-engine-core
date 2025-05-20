# Add test executable
add_executable(crawler_tests
    crawler_tests.cpp
    content_parser_tests.cpp
    robots_txt_parser_tests.cpp
    url_frontier_tests.cpp
    page_fetcher_tests.cpp
)

# Link against the crawler library and Catch2
target_link_libraries(crawler_tests
    PRIVATE
        crawler
        Catch2::Catch2WithMain
)

# Add test
catch_discover_tests(crawler_tests) 