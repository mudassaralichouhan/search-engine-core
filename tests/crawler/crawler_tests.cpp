#include <catch2/catch_test_macros.hpp>
#include "Crawler.h"
#include "models/CrawlConfig.h"
#include "models/CrawlResult.h"
#include <thread>
#include <chrono>

TEST_CASE("Basic Crawling", "[Crawler]") {
    CrawlConfig config;
    config.maxPages = 1;
    config.politenessDelay = std::chrono::milliseconds(10);
    config.userAgent = "TestBot/1.0";
    config.maxDepth = 2;
    
    Crawler crawler(config);
    
    SECTION("Starts and stops crawling") {
        crawler.addSeedURL("https://example.com");
        
        std::thread crawlerThread([&crawler]() {
            crawler.start();
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        crawler.stop();
        crawlerThread.join();
        
        auto results = crawler.getResults();
        REQUIRE_FALSE(results.empty());
    }
    
    SECTION("Respects max pages limit") {
        CrawlConfig limitedConfig;
        limitedConfig.maxPages = 2;
        limitedConfig.politenessDelay = std::chrono::milliseconds(10);
        limitedConfig.userAgent = "TestBot/1.0";
        limitedConfig.maxDepth = 1; 
        Crawler limitedCrawler(limitedConfig);
        limitedCrawler.addSeedURL("https://example.com");
        
        std::thread crawlerThread([&limitedCrawler]() {
            limitedCrawler.start();
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        limitedCrawler.stop();
        crawlerThread.join();
        
        auto results = limitedCrawler.getResults();
        REQUIRE(results.size() <= limitedConfig.maxPages);
    }
}

TEST_CASE("Seed URLs", "[Crawler]") {
    CrawlConfig config;
    config.maxPages = 2;
    config.politenessDelay = std::chrono::milliseconds(10);
    config.userAgent = "TestBot/1.0";
    config.maxDepth = 1;
    
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
        REQUIRE(results.size() >= 1);
    }
    
    SECTION("Ignores duplicate seed URLs") {
        crawler.addSeedURL("https://example.com");
        crawler.addSeedURL("https://example.com");
        
        std::thread crawlerThread([&crawler]() {
            crawler.start();
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
        crawler.stop();
        crawlerThread.join();
        
        auto results = crawler.getResults();
        long count = 0;
        for(const auto& r : results) {
            if (r.url == "https://example.com") count++;
        }
        REQUIRE(count <= 1);
        REQUIRE(results.size() <= config.maxPages);
    }
}

TEST_CASE("Robots.txt Compliance", "[Crawler]") {
    CrawlConfig config;
    config.respectRobotsTxt = true;
    config.politenessDelay = std::chrono::milliseconds(10);
    config.userAgent = "TestBot/1.0";
    config.maxPages = 5;
    config.maxDepth = 1;
    
    Crawler crawler(config);
    
    SECTION("Respects robots.txt rules") {
        crawler.addSeedURL("http://localhost:8080/private/");
        
        std::thread crawlerThread([&crawler]() {
            crawler.start();
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
        crawler.stop();
        crawlerThread.join();
        
        auto results = crawler.getResults();
        bool found_disallowed = false;
        for (const auto& result : results) {
            if (result.url.find("/private/") != std::string::npos) {
                found_disallowed = true;
                break;
            }
        }
        REQUIRE_FALSE(found_disallowed);
    }
}

TEST_CASE("Crawl Results", "[Crawler]") {
    CrawlConfig config;
    config.maxPages = 1;
    config.storeRawContent = true;
    config.extractTextContent = true;
    config.politenessDelay = std::chrono::milliseconds(10);
    config.userAgent = "TestBot/1.0";
    config.maxDepth = 1;
    
    Crawler crawler(config);
    
    SECTION("Stores crawl results correctly") {
        std::string seedUrl = "https://example.com";
        crawler.addSeedURL(seedUrl);
        
        std::thread crawlerThread([&crawler]() {
            crawler.start();
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(3));
        crawler.stop();
        crawlerThread.join();
        
        auto results = crawler.getResults();
        REQUIRE_FALSE(results.empty());
        
        if (!results.empty()) {
            const auto& result = results[0];
            REQUIRE(result.success);
            REQUIRE(result.url == seedUrl);
            REQUIRE(result.statusCode == 200);
            
            REQUIRE(result.rawContent.has_value());
            REQUIRE_FALSE(result.rawContent.value().empty());
            
            REQUIRE(result.textContent.has_value());
            REQUIRE_FALSE(result.textContent.value().empty());
            
            REQUIRE(result.title.has_value());
        }
    }
}

TEST_CASE("Error Handling", "[Crawler]") {
    CrawlConfig config;
    config.politenessDelay = std::chrono::milliseconds(10);
    config.userAgent = "TestBot/1.0";
    config.maxPages = 5;
    config.maxDepth = 1;
    
    Crawler crawler(config);
    
    SECTION("Handles invalid URLs") {
        crawler.addSeedURL("not-a-valid-url");
        
        std::thread crawlerThread([&crawler]() {
            crawler.start();
        });
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        crawler.stop();
        crawlerThread.join();
        
        auto results = crawler.getResults();
        bool found_failed_url = false;
        for(const auto& res : results) {
            if(res.url == "not-a-valid-url" && !res.success) {
                found_failed_url = true;
                break;
            }
        }
        REQUIRE(results.empty());
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
        bool found_unreachable_failed = false;
        for(const auto& res : results) {
            if(res.url == "https://nonexistent-domain-123456789.com" && !res.success) {
                found_unreachable_failed = true;
                REQUIRE(res.errorMessage.has_value());
                break;
            }
        }
        REQUIRE(found_unreachable_failed);
    }
} 