#include <catch2/catch_test_macros.hpp>
#include "URLFrontier.h"
#include <thread>
#include <chrono>

TEST_CASE("URLFrontier handles basic URL operations", "[URLFrontier]") {
    URLFrontier frontier;
    
    SECTION("Adds and retrieves URLs in order") {
        frontier.addURL("https://example.com/page1");
        frontier.addURL("https://example.com/page2");
        
        REQUIRE(frontier.size() == 2);
        REQUIRE_FALSE(frontier.isEmpty());
        
        REQUIRE(frontier.getNextURL() == "https://example.com/page1");
        REQUIRE(frontier.getNextURL() == "https://example.com/page2");
        REQUIRE(frontier.isEmpty());
    }
    
    SECTION("Handles empty frontier") {
        REQUIRE(frontier.isEmpty());
        REQUIRE(frontier.size() == 0);
        REQUIRE(frontier.getNextURL().empty());
    }
}

TEST_CASE("URLFrontier handles URL normalization", "[URLFrontier]") {
    URLFrontier frontier;
    
    SECTION("Normalizes URLs correctly") {
        frontier.addURL("https://example.com/page1");
        frontier.addURL("https://example.com/page1/"); // Trailing slash
        frontier.addURL("https://example.com/page1#section"); // Hash fragment
        
        REQUIRE(frontier.size() == 1); // Should be considered the same URL
        REQUIRE(frontier.getNextURL() == "https://example.com/page1");
    }
    
    SECTION("Handles different URL formats") {
        frontier.addURL("http://example.com");
        frontier.addURL("https://example.com");
        frontier.addURL("www.example.com");
        
        REQUIRE(frontier.size() == 3); // These should be considered different URLs
    }
}

TEST_CASE("URLFrontier handles visited URLs", "[URLFrontier]") {
    URLFrontier frontier;
    
    SECTION("Tracks visited URLs") {
        std::string url = "https://example.com/page1";
        frontier.addURL(url);
        
        REQUIRE_FALSE(frontier.isVisited(url));
        frontier.markVisited(url);
        REQUIRE(frontier.isVisited(url));
    }
    
    SECTION("Doesn't add already visited URLs") {
        std::string url = "https://example.com/page1";
        frontier.addURL(url);
        frontier.markVisited(url);
        frontier.addURL(url);
        
        REQUIRE(frontier.size() == 0);
    }
}

TEST_CASE("URLFrontier handles domain tracking", "[URLFrontier]") {
    URLFrontier frontier;
    
    SECTION("Extracts domains correctly") {
        REQUIRE(frontier.extractDomain("https://example.com/page1") == "example.com");
        REQUIRE(frontier.extractDomain("http://sub.example.com/page1") == "sub.example.com");
        REQUIRE(frontier.extractDomain("https://example.com:8080/page1") == "example.com");
    }
    
    SECTION("Tracks domain visit times") {
        std::string url = "https://example.com/page1";
        frontier.addURL(url);
        frontier.markVisited(url);
        
        auto lastVisit = frontier.getLastVisitTime("example.com");
        REQUIRE(lastVisit != std::chrono::system_clock::time_point());
        
        // Wait a bit and check if the time is updated
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        frontier.addURL("https://example.com/page2");
        frontier.markVisited("https://example.com/page2");
        
        auto newLastVisit = frontier.getLastVisitTime("example.com");
        REQUIRE(newLastVisit > lastVisit);
    }
}

TEST_CASE("URLFrontier handles concurrent access", "[URLFrontier]") {
    URLFrontier frontier;
    
    SECTION("Thread-safe URL addition and retrieval") {
        std::vector<std::thread> threads;
        std::vector<std::string> urls = {
            "https://example.com/page1",
            "https://example.com/page2",
            "https://example.com/page3"
        };
        
        // Add URLs from multiple threads
        for (const auto& url : urls) {
            threads.emplace_back([&frontier, url]() {
                frontier.addURL(url);
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(frontier.size() == urls.size());
        
        // Retrieve URLs from multiple threads
        std::vector<std::string> retrievedUrls;
        std::mutex retrievedMutex;
        
        threads.clear();
        for (size_t i = 0; i < urls.size(); ++i) {
            threads.emplace_back([&frontier, &retrievedUrls, &retrievedMutex]() {
                std::string url = frontier.getNextURL();
                if (!url.empty()) {
                    std::lock_guard<std::mutex> lock(retrievedMutex);
                    retrievedUrls.push_back(url);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(retrievedUrls.size() == urls.size());
    }
} 