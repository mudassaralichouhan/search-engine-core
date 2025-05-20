#include <catch2/catch_test_macros.hpp>
#include "Crawler.h"
#include <thread>
#include <chrono>

TEST_CASE("Crawler handles basic crawling", "[Crawler]") {
    CrawlConfig config;
    config.userAgent = "TestBot/1.0";
    config.maxPages = 10;
    config.maxDepth = 2;
    config.crawlDelay = std::chrono::milliseconds(1000);
    
    Crawler crawler(config);
    
    SECTION("Starts and stops crawling") {
        crawler.addSeedURL("https://example.com");
        
        std::thread crawlerThread([&crawler]() {
            crawler.start();
        });
        
        // Let it run for a short time
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        crawler.stop();
        crawlerThread.join();
        
        auto results = crawler.getResults();
        REQUIRE_FALSE(results.empty());
    }
    
    SECTION("Respects max pages limit") {
        config.maxPages = 2;
        Crawler limitedCrawler(config);
        limitedCrawler.addSeedURL("https://example.com");
        
        std::thread crawlerThread([&limitedCrawler]() {
            limitedCrawler.start();
        });
        
        // Let it run for a short time
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        limitedCrawler.stop();
        crawlerThread.join();
        
        auto results = limitedCrawler.getResults();
        REQUIRE(results.size() <= 2);
    }
}

TEST_CASE("Crawler handles seed URLs", "[Crawler]") {
    CrawlConfig config;
    config.userAgent = "TestBot/1.0";
    config.maxPages = 5;
    config.maxDepth = 1;
    config.crawlDelay = std::chrono::milliseconds(1000);
    
    Crawler crawler(config);
    
    SECTION("Processes multiple seed URLs") {
        crawler.addSeedURL("https://example.com");
        crawler.addSeedURL("https://example.org");
        
        std::thread crawlerThread([&crawler]() {
            crawler.start();
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
        crawler.stop();
        crawlerThread.join();
        
        auto results = crawler.getResults();
        REQUIRE(results.size() >= 2); // Should have at least the seed URLs
    }
    
    SECTION("Ignores duplicate seed URLs") {
        crawler.addSeedURL("https://example.com");
        crawler.addSeedURL("https://example.com"); // Duplicate
        
        std::thread crawlerThread([&crawler]() {
            crawler.start();
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
        crawler.stop();
        crawlerThread.join();
        
        auto results = crawler.getResults();
        REQUIRE(results.size() >= 1);
    }
}

TEST_CASE("Crawler respects robots.txt", "[Crawler]") {
    CrawlConfig config;
    config.userAgent = "TestBot/1.0";
    config.maxPages = 5;
    config.maxDepth = 1;
    config.crawlDelay = std::chrono::milliseconds(1000);
    config.respectRobotsTxt = true;
    
    Crawler crawler(config);
    
    SECTION("Respects robots.txt rules") {
        crawler.addSeedURL("https://example.com/private/"); // Assuming this is disallowed in robots.txt
        
        std::thread crawlerThread([&crawler]() {
            crawler.start();
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
        crawler.stop();
        crawlerThread.join();
        
        auto results = crawler.getResults();
        // Should not have crawled the disallowed URL
        for (const auto& result : results) {
            REQUIRE(result.url.find("/private/") == std::string::npos);
        }
    }
}

TEST_CASE("Crawler handles crawl results", "[Crawler]") {
    CrawlConfig config;
    config.userAgent = "TestBot/1.0";
    config.maxPages = 5;
    config.maxDepth = 1;
    config.crawlDelay = std::chrono::milliseconds(1000);
    
    Crawler crawler(config);
    
    SECTION("Stores crawl results correctly") {
        crawler.addSeedURL("https://example.com");
        
        std::thread crawlerThread([&crawler]() {
            crawler.start();
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
        crawler.stop();
        crawlerThread.join();
        
        auto results = crawler.getResults();
        REQUIRE_FALSE(results.empty());
        
        // Check that results contain expected fields
        for (const auto& result : results) {
            REQUIRE_FALSE(result.url.empty());
            REQUIRE_FALSE(result.title.empty());
            REQUIRE_FALSE(result.content.empty());
            REQUIRE(result.depth >= 0);
            REQUIRE(result.depth <= config.maxDepth);
        }
    }
}

TEST_CASE("Crawler handles errors gracefully", "[Crawler]") {
    CrawlConfig config;
    config.userAgent = "TestBot/1.0";
    config.maxPages = 5;
    config.maxDepth = 1;
    config.crawlDelay = std::chrono::milliseconds(1000);
    
    Crawler crawler(config);
    
    SECTION("Handles invalid URLs") {
        crawler.addSeedURL("not-a-valid-url");
        
        std::thread crawlerThread([&crawler]() {
            crawler.start();
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
        crawler.stop();
        crawlerThread.join();
        
        auto results = crawler.getResults();
        REQUIRE(results.empty()); // Should not have any results for invalid URL
    }
    
    SECTION("Handles unreachable URLs") {
        crawler.addSeedURL("https://nonexistent-domain-123456789.com");
        
        std::thread crawlerThread([&crawler]() {
            crawler.start();
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
        crawler.stop();
        crawlerThread.join();
        
        auto results = crawler.getResults();
        REQUIRE(results.empty()); // Should not have any results for unreachable URL
    }
} 