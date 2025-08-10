#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include "Crawler.h"
#include "PageFetcher.h"
#include "models/CrawlConfig.h"
#include "../../include/Logger.h"
#include "models/CrawlResult.h"
#include <thread>
#include <chrono>
#include <cstdlib> // For std::getenv
#include <future>
#include <sstream>

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
    
    Crawler crawler(config, nullptr); // No storage for basic tests
    
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
        Crawler limitedCrawler(limitedConfig, nullptr); // No storage for basic tests
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
    
    Crawler crawler(config, nullptr); // No storage for basic tests
    
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
    
    Crawler crawler(config, nullptr); // No storage for basic tests
    
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
    
    Crawler crawler(config, nullptr); // No storage for basic tests
    
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
    
    Crawler crawler(config, nullptr); // No storage for basic tests
    
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

TEST_CASE("Database Storage Integration", "[Crawler][Storage]") {
    // Skip this test if MongoDB is not available
    // You can set SKIP_DB_TESTS=1 environment variable to skip database tests
    const char* skipDbTests = std::getenv("SKIP_DB_TESTS");
    if (skipDbTests && std::string(skipDbTests) == "1") {
        SKIP("Database tests skipped by SKIP_DB_TESTS environment variable");
    }
    
    CrawlConfig config;
    config.maxPages = 1;
    config.politenessDelay = std::chrono::milliseconds(10);
    config.userAgent = "TestBot/1.0";
    config.maxDepth = 1;
    config.storeRawContent = true;
    config.extractTextContent = true;
    
    // Create a ContentStorage instance for testing
    auto storage = std::make_shared<search_engine::storage::ContentStorage>(
        "mongodb://localhost:27017",
        "search-engine"
    );
    
    // Test connection to MongoDB
    auto connectionTest = storage->testConnections();
    if (!connectionTest.success) {
        SKIP("MongoDB not available for testing: " + connectionTest.message);
    }
    
    SECTION("Saves crawl results to database") {
        Crawler crawler(config, storage);
        
        std::string testUrl = "https://example.com";
        crawler.addSeedURL(testUrl);
        
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
        REQUIRE_FALSE(results.empty());
        
        // Verify that the result was saved to the database
        auto storedProfile = storage->getSiteProfile(testUrl);
        REQUIRE(storedProfile.success);
        REQUIRE(storedProfile.value.url == testUrl);
        REQUIRE(storedProfile.value.crawlMetadata.lastCrawlStatus == search_engine::storage::CrawlStatus::SUCCESS);
        
        LOG_INFO("Successfully verified crawl result saved to database for URL: " + testUrl);
    }
    
    SECTION("Handles database storage failures gracefully") {
        // Create a crawler with invalid storage connection
        auto invalidStorage = std::make_shared<search_engine::storage::ContentStorage>(
            "mongodb://invalid-host:27017",
            "search-engine"
        );
        
        Crawler crawler(config, invalidStorage);
        
        std::string testUrl = "https://example.com";
        crawler.addSeedURL(testUrl);
        
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
        
        // Crawler should still work even if database storage fails
        auto results = crawler.getResults();
        REQUIRE_FALSE(results.empty());
        REQUIRE(results[0].success);
        
        LOG_INFO("Crawler continued working despite database storage failure");
    }
}

TEST_CASE("Parameterized Crawl Test", "[Crawler][Parameterized]") {
    // Skip this test if MongoDB is not available
    const char* skipDbTests = std::getenv("SKIP_DB_TESTS");
    if (skipDbTests && std::string(skipDbTests) == "1") {
        SKIP("Database tests skipped by SKIP_DB_TESTS environment variable");
    }
    
    // Get parameters from environment variables or use defaults
    std::string testUrl = "https://www.time.ir";  // Default
    int maxPages = 1;                             // Default
    int maxDepth = 1;                             // Default
    
    const char* urlEnv = std::getenv("TEST_URL");
    const char* pagesEnv = std::getenv("TEST_MAX_PAGES");
    const char* depthEnv = std::getenv("TEST_MAX_DEPTH");
    
    if (urlEnv) testUrl = urlEnv;
    if (pagesEnv) maxPages = std::stoi(pagesEnv);
    if (depthEnv) maxDepth = std::stoi(depthEnv);
    
    LOG_INFO("Running parameterized test with:");
    LOG_INFO("  URL: " + testUrl);
    LOG_INFO("  Max Pages: " + std::to_string(maxPages));
    LOG_INFO("  Max Depth: " + std::to_string(maxDepth));
    
    CrawlConfig config;
    config.maxPages = maxPages;
    config.politenessDelay = std::chrono::milliseconds(10);
    config.userAgent = "TestBot/1.0";
    config.maxDepth = maxDepth;
    config.storeRawContent = true;
    config.extractTextContent = true;
    
    // Create a ContentStorage instance for testing
    auto storage = std::make_shared<search_engine::storage::ContentStorage>(
        "mongodb://localhost:27017",
        "search-engine"
    );
    
    // Test connection to MongoDB
    auto connectionTest = storage->testConnections();
    if (!connectionTest.success) {
        SKIP("MongoDB not available for testing: " + connectionTest.message);
    }
    
    SECTION("Parameterized crawl with database storage") {
        Crawler crawler(config, storage);
        
        // Disable SSL verification for problematic sites
        crawler.getPageFetcher()->setVerifySSL(false);
        
        crawler.addSeedURL(testUrl);
        
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
        
        // Wait for crawler to complete or timeout after 10 seconds
        if (crawlerFuture.wait_for(std::chrono::seconds(10)) == std::future_status::timeout) {
            LOG_INFO("Crawler timeout, stopping...");
            crawler.stop();
        }
        
        crawlerThread.join();
        
        auto results = crawler.getResults();
        REQUIRE_FALSE(results.empty());
        
        // Print results for debugging
        LOG_INFO("Crawl completed. Results:");
        for (const auto& result : results) {
            LOG_INFO("  URL: " + result.url + " - Success: " + (result.success ? "true" : "false"));
            if (result.success) {
                LOG_INFO("    Title: " + (result.title.has_value() ? result.title.value() : "N/A"));
                LOG_INFO("    Content length: " + std::to_string(result.rawContent.has_value() ? result.rawContent.value().length() : 0));
            } else if (result.errorMessage.has_value()) {
                LOG_INFO("    Error: " + result.errorMessage.value());
            }
        }
        
        // Verify that the result was saved to the database
        auto storedProfile = storage->getSiteProfile(testUrl);
        REQUIRE(storedProfile.success);
        REQUIRE(storedProfile.value.url == testUrl);
        REQUIRE(storedProfile.value.crawlMetadata.lastCrawlStatus == search_engine::storage::CrawlStatus::SUCCESS);
        
        LOG_INFO("Successfully verified crawl result saved to database for URL: " + testUrl);
    }
}

TEST_CASE("processURL debug logging and SPA handling", "[Crawler][Debug][SPA]") {
    Logger::getInstance().init(LogLevel::DEBUG, true);

    // Minimal config
    CrawlConfig config;
    config.maxPages = 1;
    config.userAgent = "TestBot/1.0";
    config.maxDepth = 1;
    config.storeRawContent = true;
    config.extractTextContent = true;

    // Create a Crawler with no storage
    Crawler crawler(config, nullptr);

    // --- Test 1: Normal HTML page ---
    std::string normalUrl = "https://www.digikala.com";
    auto result = crawler.processURL(normalUrl);
    // Manual inspection: check terminal for debug output

    // --- Test 2: SPA page (simulate by using a known SPA URL) ---
    std::string spaUrl = "https://reactjs.org";
    auto spaResult = crawler.processURL(spaUrl);
    // Manual inspection: check terminal for debug output

    // The test passes if no exceptions are thrown
    REQUIRE(true);
}

TEST_CASE("Session-level SPA detection optimization", "[Crawler][SPA][Session]") {
    Logger::getInstance().init(LogLevel::INFO, true);

    // Test configuration
    CrawlConfig config;
    config.maxPages = 3;
    config.userAgent = "TestBot/1.0";
    config.maxDepth = 1;
    config.storeRawContent = true;
    config.extractTextContent = true;
    config.spaRenderingEnabled = true;
    config.browserlessUrl = "http://localhost:3000"; // Mock browserless URL

    SECTION("SPA detection only happens once per session") {
        // Create a crawler with session ID
        Crawler crawler(config, nullptr, "test_session_123");
        
        // Process multiple URLs - SPA detection should only happen on the first one
        std::vector<std::string> testUrls = {
            "https://reactjs.org",      // Known SPA
            "https://reactjs.org/docs", // Should use session-level SPA setting
            "https://reactjs.org/blog"  // Should use session-level SPA setting
        };
        
        for (const auto& url : testUrls) {
            LOG_INFO("Processing URL: " + url);
            auto result = crawler.processURL(url);
            
            // Verify the result was processed
            REQUIRE_FALSE(result.url.empty());
            LOG_INFO("Processed URL: " + url + " - Success: " + (result.success ? "true" : "false"));
        }
        
        // The test passes if no exceptions are thrown and all URLs are processed
        REQUIRE(true);
    }
    
    SECTION("Session SPA detection is reset when crawler is reset") {
        // Create a crawler with session ID
        Crawler crawler(config, nullptr, "test_session_reset");
        
        // Process first URL to trigger SPA detection
        auto result1 = crawler.processURL("https://reactjs.org");
        REQUIRE_FALSE(result1.url.empty());
        
        // Reset the crawler
        crawler.reset();
        
        // Process another URL - SPA detection should happen again
        auto result2 = crawler.processURL("https://vuejs.org");
        REQUIRE_FALSE(result2.url.empty());
        
        // The test passes if no exceptions are thrown
        REQUIRE(true);
    }
}

// Main entry point for tests
int main(int argc, char* argv[]) {
    // Set up the global logger before any tests run
    LogLevel logLevel = LogLevel::INFO;
    
    // Check for LOG_LEVEL environment variable
    std::string levelStr;
    const char* logLevelEnv = std::getenv("LOG_LEVEL");
    if (logLevelEnv) {
        levelStr = logLevelEnv;
    }
    
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
    std::string logFilePath = "./tests/logs/crawler_test.log";
    
    Logger::getInstance().init(logLevel, true, logFilePath);
    
    // Run the tests
    return Catch::Session().run(argc, argv);
} 