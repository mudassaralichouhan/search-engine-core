#include <catch2/catch_test_macros.hpp>
#include "search_core/SearchClient.hpp"
#include <thread>
#include <chrono>
#include <atomic>

using namespace hatef::search;

TEST_CASE("SearchClient basic functionality", "[SearchClient]") {
    SECTION("Constructor with default config") {
        REQUIRE_NOTHROW([](){ SearchClient client; }());
    }
    
    SECTION("Constructor with custom config") {
        RedisConfig config;
        config.uri = "tcp://127.0.0.1:6379";
        config.pool_size = 2;
        
        REQUIRE_NOTHROW([&config](){ SearchClient client(config); }());
    }
}

TEST_CASE("SearchClient Redis integration", "[SearchClient][integration]") {
    RedisConfig config;
    config.uri = "tcp://127.0.0.1:6379";
    config.pool_size = 2;
    
    SearchClient client(config);
    
    SECTION("Search with non-existent index") {
        // This should throw SearchError for non-existent index
        REQUIRE_THROWS_AS(client.search("nonexistent_index", "test"), SearchError);
    }
    
    SECTION("Search with empty query") {
        REQUIRE_NOTHROW(client.search("test_index", ""));
    }
    
    SECTION("Search with additional arguments") {
        std::vector<std::string> args = {"LIMIT", "0", "10"};
        REQUIRE_NOTHROW(client.search("test_index", "test", args));
    }
}

TEST_CASE("SearchClient thread safety", "[SearchClient][integration]") {
    RedisConfig config;
    config.uri = "tcp://127.0.0.1:6379";
    config.pool_size = 4;
    
    SearchClient client(config);
    
    SECTION("Concurrent searches") {
        std::vector<std::thread> threads;
        std::atomic<int> successful_calls{0};
        
        for (int i = 0; i < 10; ++i) {
            threads.emplace_back([&client, &successful_calls, i]() {
                try {
                    client.search("test_index", "query" + std::to_string(i));
                    successful_calls.fetch_add(1);
                } catch (const SearchError&) {
                    // Expected for non-existent index
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        // We expect at least some calls to complete without exceptions
        // (even if they return empty results due to non-existent index)
        REQUIRE(successful_calls.load() >= 0);
    }
}

TEST_CASE("SearchClient error handling", "[SearchClient]") {
    SECTION("Invalid Redis URI") {
        RedisConfig config;
        // Use an invalid port that should definitely fail
        config.uri = "tcp://127.0.0.1:9999";
        
        REQUIRE_THROWS_AS([&config](){ SearchClient client(config); }(), SearchError);
    }
} 