#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include "../../include/search_engine/storage/ContentStorage.h"
#include "../../src/crawler/models/CrawlResult.h"
#include <chrono>
#include <thread>

using namespace search_engine::storage;

// Test data helpers
namespace {
    CrawlResult createTestCrawlResult(const std::string& url = "https://example.com") {
        CrawlResult result;
        result.url = url;
        result.statusCode = 200;
        result.contentType = "text/html";
        result.rawContent = "<html><head><title>Test Page</title></head><body><h1>Test Content</h1><p>This is test content for the search engine.</p></body></html>";
        result.textContent = "Test Page Test Content This is test content for the search engine.";
        result.title = "Test Page";
        result.metaDescription = "A test page for unit testing";
        result.links = {"https://example.org", "https://test.com"};
        result.crawlTime = std::chrono::system_clock::now();
        result.contentSize = result.rawContent->length();
        result.success = true;
        
        return result;
    }
}

TEST_CASE("Content Storage - Initialization and Connection", "[content][storage]") {
    SECTION("Constructor with default parameters") {
        REQUIRE_NOTHROW(ContentStorage storage);
    }
    
    SECTION("Constructor with custom parameters") {
        REQUIRE_NOTHROW(ContentStorage storage(
            "mongodb://localhost:27017",
            "test-search-engine",
            "tcp://127.0.0.1:6379",
            "test_content_index"
        ));
    }
    
    SECTION("Test connections") {
        ContentStorage storage(
            "mongodb://localhost:27017",
            "test-search-engine",
            "tcp://127.0.0.1:6379",
            "test_content_index"
        );
        
        auto result = storage.testConnections();
        if (result.isSuccess()) {
            REQUIRE(result.getValue() == true);
            REQUIRE(result.getMessage() == "All connections are healthy");
        } else {
            WARN("Storage connections not available: " + result.getErrorMessage());
            return;
        }
    }
}

TEST_CASE("Content Storage - Crawl Result Processing", "[content][storage][crawl]") {
    ContentStorage storage(
        "mongodb://localhost:27017",
        "test-search-engine",
        "tcp://127.0.0.1:6379",
        "test_content_index"
    );
    
    // Skip tests if connections are not available
    auto connectionTest = storage.testConnections();
    if (!connectionTest.isSuccess()) {
        WARN("Skipping content storage tests - connections not available");
        return;
    }
    
    SECTION("Store single crawl result") {
        CrawlResult testResult = createTestCrawlResult("https://test-content.com");
        
        // Store the crawl result
        auto storeResult = storage.storeCrawlResult(testResult);
        REQUIRE(storeResult.isSuccess());
        REQUIRE(!storeResult.getValue().empty());
        
        std::string profileId = storeResult.getValue();
        
        // Retrieve the site profile
        auto profileResult = storage.getSiteProfile("https://test-content.com");
        REQUIRE(profileResult.isSuccess());
        
        SiteProfile profile = profileResult.getValue();
        REQUIRE(profile.url == testResult.url);
        REQUIRE(profile.title == testResult.title.value_or(""));
        REQUIRE(profile.description == testResult.metaDescription);
        REQUIRE(profile.outboundLinks == testResult.links);
        REQUIRE(profile.crawlMetadata.httpStatusCode == testResult.statusCode);
        REQUIRE(profile.crawlMetadata.contentSize == testResult.contentSize);
        REQUIRE(profile.crawlMetadata.lastCrawlStatus == CrawlStatus::SUCCESS);
        
        // Test search functionality
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        auto searchResult = storage.searchSimple("test content", 10);
        REQUIRE(searchResult.isSuccess());
        
        auto response = searchResult.getValue();
        bool found = false;
        for (const auto& result : response.results) {
            if (result.url == "https://test-content.com") {
                found = true;
                break;
            }
        }
        REQUIRE(found);
        
        // Clean up
        storage.deleteSiteData("https://test-content.com");
    }
    
    SECTION("Update existing crawl result") {
        CrawlResult initialResult = createTestCrawlResult("https://test-update-content.com");
        
        // Store initial result
        auto storeResult = storage.storeCrawlResult(initialResult);
        REQUIRE(storeResult.isSuccess());
        
        // Create updated result
        CrawlResult updatedResult = initialResult;
        updatedResult.title = "Updated Test Page";
        updatedResult.textContent = "Updated test content for the search engine.";
        updatedResult.crawlTime = std::chrono::system_clock::now();
        
        // Update the result
        auto updateResult = storage.updateCrawlResult(updatedResult);
        REQUIRE(updateResult.isSuccess());
        
        // Verify the update
        auto profileResult = storage.getSiteProfile("https://test-update-content.com");
        REQUIRE(profileResult.isSuccess());
        
        SiteProfile profile = profileResult.getValue();
        REQUIRE(profile.title == "Updated Test Page");
        REQUIRE(profile.crawlMetadata.crawlCount == 2); // Should be incremented
        
        // Clean up
        storage.deleteSiteData("https://test-update-content.com");
    }
    
    SECTION("Store failed crawl result") {
        CrawlResult failedResult = createTestCrawlResult("https://test-failed.com");
        failedResult.success = false;
        failedResult.statusCode = 404;
        failedResult.errorMessage = "Page not found";
        failedResult.textContent = std::nullopt;
        failedResult.rawContent = std::nullopt;
        
        // Store the failed result
        auto storeResult = storage.storeCrawlResult(failedResult);
        REQUIRE(storeResult.isSuccess());
        
        // Verify the profile
        auto profileResult = storage.getSiteProfile("https://test-failed.com");
        REQUIRE(profileResult.isSuccess());
        
        SiteProfile profile = profileResult.getValue();
        REQUIRE(profile.crawlMetadata.lastCrawlStatus == CrawlStatus::FAILED);
        REQUIRE(profile.crawlMetadata.lastErrorMessage == "Page not found");
        REQUIRE(profile.crawlMetadata.httpStatusCode == 404);
        REQUIRE(!profile.isIndexed);
        
        // Clean up
        storage.deleteSiteData("https://test-failed.com");
    }
}

TEST_CASE("Content Storage - Batch Operations", "[content][storage][batch]") {
    ContentStorage storage(
        "mongodb://localhost:27017",
        "test-search-engine",
        "tcp://127.0.0.1:6379",
        "test_batch_index"
    );
    
    // Skip tests if connections are not available
    auto connectionTest = storage.testConnections();
    if (!connectionTest.isSuccess()) {
        WARN("Skipping content storage tests - connections not available");
        return;
    }
    
    SECTION("Store multiple crawl results") {
        std::vector<CrawlResult> crawlResults;
        for (int i = 0; i < 3; ++i) {
            CrawlResult result = createTestCrawlResult("https://batch" + std::to_string(i) + ".com");
            result.title = "Batch Test Page " + std::to_string(i);
            result.textContent = "Batch test content number " + std::to_string(i) + " for testing.";
            crawlResults.push_back(result);
        }
        
        // Store all results
        auto batchResult = storage.storeCrawlResults(crawlResults);
        REQUIRE(batchResult.isSuccess());
        
        auto ids = batchResult.getValue();
        REQUIRE(ids.size() == 3);
        
        // Verify all were stored
        for (int i = 0; i < 3; ++i) {
            auto profileResult = storage.getSiteProfile("https://batch" + std::to_string(i) + ".com");
            REQUIRE(profileResult.isSuccess());
            
            SiteProfile profile = profileResult.getValue();
            REQUIRE(profile.title == "Batch Test Page " + std::to_string(i));
        }
        
        // Test search across all documents
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        auto searchResult = storage.searchSimple("batch test", 10);
        REQUIRE(searchResult.isSuccess());
        REQUIRE(searchResult.getValue().results.size() >= 3);
        
        // Clean up
        for (int i = 0; i < 3; ++i) {
            storage.deleteSiteData("https://batch" + std::to_string(i) + ".com");
        }
    }
}

TEST_CASE("Content Storage - Domain Operations", "[content][storage][domain]") {
    ContentStorage storage(
        "mongodb://localhost:27017",
        "test-search-engine",
        "tcp://127.0.0.1:6379",
        "test_domain_index"
    );
    
    // Skip tests if connections are not available
    auto connectionTest = storage.testConnections();
    if (!connectionTest.isSuccess()) {
        WARN("Skipping content storage tests - connections not available");
        return;
    }
    
    SECTION("Get profiles by domain") {
        // Create multiple pages for the same domain
        std::vector<CrawlResult> domainResults;
        for (int i = 0; i < 3; ++i) {
            CrawlResult result = createTestCrawlResult("https://testdomain.com/page" + std::to_string(i));
            result.title = "Domain Page " + std::to_string(i);
            domainResults.push_back(result);
        }
        
        // Store all results
        for (const auto& result : domainResults) {
            auto storeResult = storage.storeCrawlResult(result);
            REQUIRE(storeResult.isSuccess());
        }
        
        // Get profiles by domain
        auto domainProfilesResult = storage.getSiteProfilesByDomain("testdomain.com");
        REQUIRE(domainProfilesResult.isSuccess());
        REQUIRE(domainProfilesResult.getValue().size() >= 3);
        
        // Clean up domain data
        auto deleteResult = storage.deleteDomainData("testdomain.com");
        REQUIRE(deleteResult.isSuccess());
        
        // Verify deletion
        auto verifyResult = storage.getSiteProfilesByDomain("testdomain.com");
        REQUIRE(verifyResult.isSuccess());
        REQUIRE(verifyResult.getValue().empty());
    }
}

TEST_CASE("Content Storage - Search Operations", "[content][storage][search]") {
    ContentStorage storage(
        "mongodb://localhost:27017",
        "test-search-engine",
        "tcp://127.0.0.1:6379",
        "test_search_index"
    );
    
    // Skip tests if connections are not available
    auto connectionTest = storage.testConnections();
    if (!connectionTest.isSuccess()) {
        WARN("Skipping content storage tests - connections not available");
        return;
    }
    
    SECTION("Advanced search with filters") {
        // Create documents with different categories
        CrawlResult techResult = createTestCrawlResult("https://tech-search.com");
        techResult.title = "Technology News";
        techResult.textContent = "Latest technology trends and innovations.";
        
        CrawlResult newsResult = createTestCrawlResult("https://news-search.com");
        newsResult.title = "Breaking News";
        newsResult.textContent = "Latest news updates from around the world.";
        
        // Store both results
        storage.storeCrawlResult(techResult);
        storage.storeCrawlResult(newsResult);
        
        // Give time for indexing
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        // Test simple search
        auto simpleResult = storage.searchSimple("technology", 10);
        REQUIRE(simpleResult.isSuccess());
        REQUIRE(simpleResult.getValue().results.size() >= 1);
        
        // Test advanced search with query object
        SearchQuery query;
        query.query = "news";
        query.limit = 5;
        query.highlight = true;
        
        auto advancedResult = storage.search(query);
        REQUIRE(advancedResult.isSuccess());
        REQUIRE(advancedResult.getValue().results.size() >= 1);
        
        // Clean up
        storage.deleteSiteData("https://tech-search.com");
        storage.deleteSiteData("https://news-search.com");
    }
    
    SECTION("Search suggestions") {
        // Create a document with specific terms
        CrawlResult suggestionResult = createTestCrawlResult("https://suggestion-test.com");
        suggestionResult.title = "Suggestion Testing Document";
        suggestionResult.textContent = "This document is for testing search suggestions and autocomplete.";
        
        storage.storeCrawlResult(suggestionResult);
        
        // Give time for indexing
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Test suggestions (note: this might not work with all Redis configurations)
        auto suggestResult = storage.suggest("sugg", 5);
        // Suggestions might not be available in all Redis setups, so we just check it doesn't crash
        REQUIRE_NOTHROW(storage.suggest("test", 5));
        
        // Clean up
        storage.deleteSiteData("https://suggestion-test.com");
    }
}

TEST_CASE("Content Storage - Statistics and Monitoring", "[content][storage][stats]") {
    ContentStorage storage(
        "mongodb://localhost:27017",
        "test-search-engine",
        "tcp://127.0.0.1:6379",
        "test_stats_index"
    );
    
    // Skip tests if connections are not available
    auto connectionTest = storage.testConnections();
    if (!connectionTest.isSuccess()) {
        WARN("Skipping content storage tests - connections not available");
        return;
    }
    
    SECTION("Get storage statistics") {
        auto statsResult = storage.getStorageStats();
        REQUIRE(statsResult.isSuccess());
        
        auto stats = statsResult.getValue();
        REQUIRE(!stats.empty());
        
        // Should have MongoDB stats
        REQUIRE(stats.find("mongodb_total_documents") != stats.end());
        
        // Should have Redis stats
        REQUIRE(stats.find("redis_indexed_documents") != stats.end());
    }
    
    SECTION("Get total site count") {
        auto initialCount = storage.getTotalSiteCount();
        REQUIRE(initialCount.isSuccess());
        
        // Add a test site
        CrawlResult testResult = createTestCrawlResult("https://count-test.com");
        auto storeResult = storage.storeCrawlResult(testResult);
        REQUIRE(storeResult.isSuccess());
        
        // Check count increased
        auto newCount = storage.getTotalSiteCount();
        REQUIRE(newCount.isSuccess());
        REQUIRE(newCount.getValue() == initialCount.getValue() + 1);
        
        // Clean up
        storage.deleteSiteData("https://count-test.com");
    }
    
    SECTION("Get profiles by crawl status") {
        // Create profiles with different statuses
        CrawlResult successResult = createTestCrawlResult("https://success-status.com");
        CrawlResult failedResult = createTestCrawlResult("https://failed-status.com");
        failedResult.success = false;
        failedResult.statusCode = 500;
        failedResult.errorMessage = "Server error";
        
        storage.storeCrawlResult(successResult);
        storage.storeCrawlResult(failedResult);
        
        // Get profiles by status
        auto successProfiles = storage.getSiteProfilesByCrawlStatus(CrawlStatus::SUCCESS);
        auto failedProfiles = storage.getSiteProfilesByCrawlStatus(CrawlStatus::FAILED);
        
        REQUIRE(successProfiles.isSuccess());
        REQUIRE(failedProfiles.isSuccess());
        REQUIRE(successProfiles.getValue().size() >= 1);
        REQUIRE(failedProfiles.getValue().size() >= 1);
        
        // Clean up
        storage.deleteSiteData("https://success-status.com");
        storage.deleteSiteData("https://failed-status.com");
    }
}

TEST_CASE("Content Storage - Index Management", "[content][storage][index]") {
    ContentStorage storage(
        "mongodb://localhost:27017",
        "test-search-engine",
        "tcp://127.0.0.1:6379",
        "test_index_mgmt"
    );
    
    // Skip tests if connections are not available
    auto connectionTest = storage.testConnections();
    if (!connectionTest.isSuccess()) {
        WARN("Skipping content storage tests - connections not available");
        return;
    }
    
    SECTION("Initialize indexes") {
        auto initResult = storage.initializeIndexes();
        REQUIRE(initResult.isSuccess());
    }
    
    SECTION("Test direct storage access") {
        // Test that we can get direct access to underlying storage layers
        auto mongoStorage = storage.getMongoStorage();
        auto redisStorage = storage.getRedisStorage();
        
        REQUIRE(mongoStorage != nullptr);
        REQUIRE(redisStorage != nullptr);
        
        // Test direct operations
        auto mongoTest = mongoStorage->testConnection();
        auto redisTest = redisStorage->testConnection();
        
        if (mongoTest.isSuccess()) {
            REQUIRE(mongoTest.getValue() == true);
        }
        if (redisTest.isSuccess()) {
            REQUIRE(redisTest.getValue() == true);
        }
    }
}

TEST_CASE("Content Storage - Error Handling", "[content][storage][errors]") {
    ContentStorage storage(
        "mongodb://localhost:27017",
        "test-search-engine",
        "tcp://127.0.0.1:6379",
        "test_error_index"
    );
    
    // Skip tests if connections are not available
    auto connectionTest = storage.testConnections();
    if (!connectionTest.isSuccess()) {
        WARN("Skipping content storage tests - connections not available");
        return;
    }
    
    SECTION("Delete non-existent site data") {
        auto deleteResult = storage.deleteSiteData("https://non-existent-site.com");
        REQUIRE(!deleteResult.isSuccess());
    }
    
    SECTION("Get non-existent site profile") {
        auto profileResult = storage.getSiteProfile("https://non-existent-profile.com");
        REQUIRE(!profileResult.isSuccess());
    }
    
    SECTION("Get profiles for non-existent domain") {
        auto domainResult = storage.getSiteProfilesByDomain("non-existent-domain.com");
        REQUIRE(domainResult.isSuccess());
        REQUIRE(domainResult.getValue().empty());
    }
} 