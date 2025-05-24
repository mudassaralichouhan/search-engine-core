#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include "Crawler.h"
#include "models/CrawlConfig.h"
#include "../../include/Logger.h"
#include "models/CrawlResult.h"
#include <thread>
#include <chrono>
#include <cstdlib> // For std::getenv
#include <future>

// Helper function to print results details with HTTP status codes
void printResults(const std::vector<CrawlResult>& results) {
    LOG_INFO("===== CRAWL RESULTS DETAILS =====");
    LOG_INFO("Total results: " + std::to_string(results.size()));
    
    for (size_t i = 0; i < results.size(); i++) {
        const auto& result = results[i];
        LOG_INFO_STREAM("Result #" << (i+1) << ":");
        LOG_INFO_STREAM("  URL: " << result.url);
        LOG_INFO_STREAM("  Success: " << (result.success ? "Yes" : "No"));
        LOG_INFO_STREAM("  HTTP Status: " << result.statusCode);
        if (!result.success && result.errorMessage.has_value()) {
            LOG_INFO_STREAM("  Error: " << result.errorMessage.value());
        }
        if (result.title.has_value()) {
            LOG_INFO_STREAM("  Title: " << result.title.value());
        }
        LOG_INFO_STREAM("  Content Type: " << result.contentType);
        LOG_INFO_STREAM("  Content Size: " << result.contentSize << " bytes");
        LOG_INFO("");
    }
    LOG_INFO("=================================");
}

TEST_CASE("Basic Crawling", "[Crawler]") {
    // Log level is already set by the main function
    LogLevel logLevel = Logger::getInstance().isEnabled(LogLevel::INFO) ? LogLevel::INFO : LogLevel::NONE;
    
    if (logLevel != LogLevel::NONE) {
        LOG_INFO("Starting Basic Crawling test");
    }
    
    CrawlConfig config;
    config.maxPages = 1;
    config.politenessDelay = std::chrono::milliseconds(10);
    config.userAgent = "TestBot/1.0";
    config.maxDepth = 1;
    
    Crawler crawler(config);
    
    SECTION("Starts and stops crawling") {
        LOG_INFO("Adding seed URL: https://example.com");
        crawler.addSeedURL("https://example.com");
        
        // Create a promise and future for synchronization
        std::promise<void> crawlerPromise;
        std::future<void> crawlerFuture = crawlerPromise.get_future();
        
        LOG_INFO("Starting crawler thread");
        std::thread crawlerThread([&crawler, &crawlerPromise]() {
            LOG_DEBUG("Inside crawler thread, calling start()");
            crawler.start();
            LOG_DEBUG("Crawler start() returned");
            
            // Wait for crawler to finish processing
            while (crawler.getResults().empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            crawlerPromise.set_value(); // Signal that crawler has finished
        });
        
        // Wait for crawler to complete or timeout after 5 seconds
        if (crawlerFuture.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
            LOG_INFO("Crawler timeout, stopping...");
            crawler.stop();
        }
        
        LOG_INFO("Joining thread");
        crawlerThread.join();
        LOG_INFO("Thread joined");
        
        LOG_INFO("Getting results");
        auto results = crawler.getResults();
        LOG_INFO("Results size: " + std::to_string(results.size()));
        
        // Print detailed results including HTTP status codes
        printResults(results);
        
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
        
        // Create a promise and future for synchronization
        std::promise<void> crawlerPromise;
        std::future<void> crawlerFuture = crawlerPromise.get_future();
        
        std::thread crawlerThread([&limitedCrawler, &crawlerPromise]() {
            limitedCrawler.start();
            
            // Wait for crawler to finish processing
            while (limitedCrawler.getResults().empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            crawlerPromise.set_value(); // Signal that crawler has finished
        });
        
        // Wait for crawler to complete or timeout after 5 seconds
        if (crawlerFuture.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
            LOG_INFO("Crawler timeout, stopping...");
            limitedCrawler.stop();
        }
        
        crawlerThread.join();
        
        auto results = limitedCrawler.getResults();
        REQUIRE(results.size() <= limitedConfig.maxPages);
    }
}

TEST_CASE("Seed URLs", "[Crawler]") {
    // Initialize logger for the test
    Logger::getInstance().init(LogLevel::INFO, true);
    
    CrawlConfig config;
    config.maxPages = 2;  // Allow processing both URLs
    config.politenessDelay = std::chrono::milliseconds(10);
    config.userAgent = "TestBot/1.0";
    config.maxDepth = 1;
    
    Crawler crawler(config);
    
    SECTION("Processes multiple seed URLs") {
        crawler.addSeedURL("https://example.com");
        crawler.addSeedURL("https://www.google.com");
        
        // Create a promise and future for synchronization
        std::promise<void> crawlerPromise;
        std::future<void> crawlerFuture = crawlerPromise.get_future();
        
        std::thread crawlerThread([&crawler, &crawlerPromise]() {
            crawler.start();
            
            // Wait for crawler to finish processing
            while (crawler.getResults().empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            crawlerPromise.set_value(); // Signal that crawler has finished
        });
        
        // Wait for crawler to complete or timeout after 30 seconds
        if (crawlerFuture.wait_for(std::chrono::seconds(30)) == std::future_status::timeout) {
            LOG_INFO("Crawler timeout, stopping...");
            crawler.stop();
        }
        
        crawlerThread.join();
        
        auto results = crawler.getResults();
        LOG_INFO("Test results size: " + std::to_string(results.size()));
        REQUIRE(results.size() >= 1);
        
        // Verify that at least one of the seed URLs was processed
        bool foundSeedURL = false;
        for (const auto& result : results) {
            if (result.url == "https://example.com" || result.url == "https://www.google.com") {
                foundSeedURL = true;
                break;
            }
        }
        REQUIRE(foundSeedURL);
    }
    
    SECTION("Ignores duplicate seed URLs") {
        crawler.addSeedURL("https://example.com");
        crawler.addSeedURL("https://example.com");
        
        // Create a promise and future for synchronization
        std::promise<void> crawlerPromise;
        std::future<void> crawlerFuture = crawlerPromise.get_future();
        
        std::thread crawlerThread([&crawler, &crawlerPromise]() {
            crawler.start();
            
            // Wait for crawler to finish processing
            while (crawler.getResults().empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            crawlerPromise.set_value(); // Signal that crawler has finished
        });
        
        // Wait for crawler to complete or timeout after 5 seconds
        if (crawlerFuture.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
            LOG_INFO("Crawler timeout, stopping...");
            crawler.stop();
        }
        
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
        
        // Create a promise and future for synchronization
        std::promise<void> crawlerPromise;
        std::future<void> crawlerFuture = crawlerPromise.get_future();
        
        std::thread crawlerThread([&crawler, &crawlerPromise]() {
            crawler.start();
            
            // Wait for crawler to finish processing
            while (crawler.getResults().empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            crawlerPromise.set_value(); // Signal that crawler has finished
        });
        
        // Wait for crawler to complete or timeout after 2 seconds
        if (crawlerFuture.wait_for(std::chrono::seconds(2)) == std::future_status::timeout) {
            LOG_INFO("Crawler timeout, stopping...");
            crawler.stop();
        }
        
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
        
        // Create a promise and future for synchronization
        std::promise<void> crawlerPromise;
        std::future<void> crawlerFuture = crawlerPromise.get_future();
        
        std::thread crawlerThread([&crawler, &crawlerPromise]() {
            crawler.start();
            
            // Wait for crawler to finish processing
            while (crawler.getResults().empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            crawlerPromise.set_value(); // Signal that crawler has finished
        });
        
        // Wait for crawler to complete or timeout after 3 seconds
        if (crawlerFuture.wait_for(std::chrono::seconds(3)) == std::future_status::timeout) {
            LOG_INFO("Crawler timeout, stopping...");
            crawler.stop();
        }
        
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
        
        // Create a promise and future for synchronization
        std::promise<void> crawlerPromise;
        std::future<void> crawlerFuture = crawlerPromise.get_future();
        
        std::thread crawlerThread([&crawler, &crawlerPromise]() {
            crawler.start();
            
            // Wait for crawler to finish processing
            while (crawler.getResults().empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            crawlerPromise.set_value(); // Signal that crawler has finished
        });
        
        // Wait for crawler to complete or timeout after 1 second
        if (crawlerFuture.wait_for(std::chrono::seconds(1)) == std::future_status::timeout) {
            LOG_INFO("Crawler timeout, stopping...");
            crawler.stop();
        }
        
        crawlerThread.join();
        
        auto results = crawler.getResults();
        bool found_failed_url = false;
        for(const auto& res : results) {
            if(res.url == "not-a-valid-url" && !res.success) {
                found_failed_url = true;
                REQUIRE(res.errorMessage.has_value());
                REQUIRE_FALSE(res.errorMessage.value().empty());
                break;
            }
        }
        REQUIRE(found_failed_url);
    }
    
    SECTION("Handles unreachable URLs") {
        crawler.addSeedURL("https://nonexistent-domain-123456789.com");
        
        // Create a promise and future for synchronization
        std::promise<void> crawlerPromise;
        std::future<void> crawlerFuture = crawlerPromise.get_future();
        
        std::thread crawlerThread([&crawler, &crawlerPromise]() {
            crawler.start();
            
            // Wait for crawler to finish processing
            while (crawler.getResults().empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            crawlerPromise.set_value(); // Signal that crawler has finished
        });
        
        // Wait for crawler to complete or timeout after 2 seconds
        if (crawlerFuture.wait_for(std::chrono::seconds(2)) == std::future_status::timeout) {
            LOG_INFO("Crawler timeout, stopping...");
            crawler.stop();
        }
        
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

// Main entry point for tests
int main(int argc, char* argv[]) {
    // Set up the global logger before any tests run
    LogLevel logLevel = LogLevel::INFO;
    
    // Check for LOG_LEVEL environment variable
    // Using a cross-platform safe approach to get environment variables
    std::string levelStr;
    #ifdef _WIN32
        // Windows-specific secure version
        char* buffer = nullptr;
        size_t size = 0;
        if (_dupenv_s(&buffer, &size, "LOG_LEVEL") == 0 && buffer != nullptr) {
            levelStr = buffer;
            free(buffer);
        }
    #else
        // POSIX systems (Linux, macOS)
        const char* logLevelEnv = std::getenv("LOG_LEVEL");
        if (logLevelEnv) {
            levelStr = logLevelEnv;
        }
    #endif
    
    if (!levelStr.empty()) {
        if (levelStr == "NONE") {
            logLevel = LogLevel::NONE;
        } else if (levelStr == "ERROR") {
            logLevel = LogLevel::ERR; 
        } else if (levelStr == "WARNING") {
            logLevel = LogLevel::WARNING;
        } else if (levelStr == "INFO") {
            logLevel = LogLevel::INFO;
        } else if (levelStr == "DEBUG") {
            logLevel = LogLevel::DEBUG;
        } else if (levelStr == "TRACE") {
            logLevel = LogLevel::TRACE;
        }
    }
    
    // Initialize the global logger
    // Use a cross-platform path for the log file
    #ifdef _WIN32
        std::string logFilePath = "D:/Projects/hatef.ir/search-engine-core/tests/logs/crawler_test.log";
    #else
        std::string logFilePath = "./tests/logs/crawler_test.log";
    #endif
    
    Logger::getInstance().init(logLevel, true, logFilePath);
    
    // Run the tests
    return Catch::Session().run(argc, argv);
} 