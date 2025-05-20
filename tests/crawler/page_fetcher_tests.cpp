#include <catch2/catch_test_macros.hpp>
#include "PageFetcher.h"
#include <thread>
#include <chrono>

TEST_CASE("PageFetcher handles basic page fetching", "[PageFetcher]") {
    PageFetcher fetcher("TestBot/1.0", std::chrono::seconds(10));
    
    SECTION("Fetches valid page") {
        auto result = fetcher.fetch("https://example.com");
        
        REQUIRE(result.success);
        REQUIRE(result.statusCode == 200);
        REQUIRE(result.contentType.find("text/html") != std::string::npos);
        REQUIRE_FALSE(result.content.empty());
        REQUIRE(result.finalUrl == "https://example.com");
        REQUIRE(result.errorMessage.empty());
    }
    
    SECTION("Handles non-existent page") {
        auto result = fetcher.fetch("https://example.com/nonexistent");
        
        REQUIRE_FALSE(result.success);
        REQUIRE(result.statusCode == 404);
        REQUIRE(result.errorMessage.empty());
    }
}

TEST_CASE("PageFetcher handles redirects", "[PageFetcher]") {
    PageFetcher fetcher("TestBot/1.0", std::chrono::seconds(10));
    
    SECTION("Follows redirects when enabled") {
        auto result = fetcher.fetch("http://example.com"); // Will redirect to https
        
        REQUIRE(result.success);
        REQUIRE(result.statusCode == 200);
        REQUIRE(result.finalUrl == "https://example.com");
    }
    
    SECTION("Respects max redirects limit") {
        PageFetcher limitedFetcher("TestBot/1.0", std::chrono::seconds(10), true, 1);
        // Note: This test might need adjustment based on actual redirect behavior
        auto result = limitedFetcher.fetch("http://example.com");
        
        REQUIRE(result.success);
        REQUIRE(result.statusCode == 200);
    }
}

TEST_CASE("PageFetcher handles timeouts", "[PageFetcher]") {
    SECTION("Respects timeout setting") {
        PageFetcher fastFetcher("TestBot/1.0", std::chrono::milliseconds(100));
        auto result = fastFetcher.fetch("https://example.com");
        
        // The page should still load within 100ms, but if it doesn't:
        if (!result.success) {
            REQUIRE(result.errorMessage.find("timeout") != std::string::npos);
        }
    }
}

TEST_CASE("PageFetcher handles custom headers", "[PageFetcher]") {
    PageFetcher fetcher("TestBot/1.0", std::chrono::seconds(10));
    
    SECTION("Sets custom headers correctly") {
        std::vector<std::pair<std::string, std::string>> headers = {
            {"Accept-Language", "en-US"},
            {"Cache-Control", "no-cache"}
        };
        
        fetcher.setCustomHeaders(headers);
        auto result = fetcher.fetch("https://example.com");
        
        REQUIRE(result.success);
        REQUIRE(result.statusCode == 200);
    }
}

TEST_CASE("PageFetcher handles progress callbacks", "[PageFetcher]") {
    PageFetcher fetcher("TestBot/1.0", std::chrono::seconds(10));
    bool callbackCalled = false;
    size_t totalBytes = 0;
    size_t downloadedBytes = 0;
    
    SECTION("Calls progress callback during download") {
        fetcher.setProgressCallback([&](size_t total, size_t downloaded) {
            callbackCalled = true;
            totalBytes = total;
            downloadedBytes = downloaded;
        });
        
        auto result = fetcher.fetch("https://example.com");
        
        REQUIRE(result.success);
        REQUIRE(callbackCalled);
        REQUIRE(totalBytes > 0);
        REQUIRE(downloadedBytes > 0);
        REQUIRE(downloadedBytes <= totalBytes);
    }
}

TEST_CASE("PageFetcher handles SSL verification", "[PageFetcher]") {
    SECTION("Verifies SSL by default") {
        PageFetcher fetcher("TestBot/1.0", std::chrono::seconds(10));
        auto result = fetcher.fetch("https://example.com");
        
        REQUIRE(result.success);
    }
    
    SECTION("Can disable SSL verification") {
        PageFetcher fetcher("TestBot/1.0", std::chrono::seconds(10));
        fetcher.setVerifySSL(false);
        auto result = fetcher.fetch("https://example.com");
        
        REQUIRE(result.success);
    }
}

TEST_CASE("PageFetcher handles proxy settings", "[PageFetcher]") {
    PageFetcher fetcher("TestBot/1.0", std::chrono::seconds(10));
    
    SECTION("Can set proxy") {
        fetcher.setProxy("http://proxy.example.com:8080");
        auto result = fetcher.fetch("https://example.com");
        
        // Note: This test might fail if the proxy is not available
        // In a real test environment, you might want to use a mock proxy
        if (!result.success) {
            REQUIRE(result.errorMessage.find("proxy") != std::string::npos);
        }
    }
} 