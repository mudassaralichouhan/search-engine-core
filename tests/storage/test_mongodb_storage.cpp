#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include "../../include/search_engine/storage/MongoDBStorage.h"
#include <chrono>
#include <thread>

using namespace search_engine::storage;

// Test data helpers
namespace {
    SiteProfile createTestSiteProfile(const std::string& url = "https://example.com") {
        SiteProfile profile;
        profile.domain = "example.com";
        profile.url = url;
        profile.title = "Test Site";
        profile.description = "A test website for unit testing";
        profile.keywords = {"test", "example", "website"};
        profile.language = "en";
        profile.category = "technology";
        
        // Crawl metadata
        auto now = std::chrono::system_clock::now();
        profile.crawlMetadata.lastCrawlTime = now;
        profile.crawlMetadata.firstCrawlTime = now;
        profile.crawlMetadata.lastCrawlStatus = CrawlStatus::SUCCESS;
        profile.crawlMetadata.crawlCount = 1;
        profile.crawlMetadata.crawlIntervalHours = 24.0;
        profile.crawlMetadata.userAgent = "TestBot/1.0";
        profile.crawlMetadata.httpStatusCode = 200;
        profile.crawlMetadata.contentSize = 5000;
        profile.crawlMetadata.contentType = "text/html";
        profile.crawlMetadata.crawlDurationMs = 250.5;
        
        // SEO metrics
        profile.pageRank = 5;
        profile.contentQuality = 0.8;
        profile.wordCount = 500;
        profile.isMobile = true;
        profile.hasSSL = true;
        
        // Links
        profile.outboundLinks = {"https://example.org", "https://test.com"};
        profile.inboundLinkCount = 10;
        
        // Search relevance
        profile.isIndexed = true;
        profile.lastModified = now;
        profile.indexedAt = now;
        
        // Additional metadata
        profile.author = "John Doe";
        profile.publisher = "Example Corp";
        profile.publishDate = now - std::chrono::hours(24);
        
        return profile;
    }
}

TEST_CASE("MongoDB Storage - Connection and Initialization", "[mongodb][storage]") {
    SECTION("Constructor with default parameters") {
        REQUIRE_NOTHROW(MongoDBStorage());
    }
    
    SECTION("Constructor with custom parameters") {
        REQUIRE_NOTHROW(MongoDBStorage("mongodb://localhost:27017", "test-db"));
    }
    
    SECTION("Test connection") {
        MongoDBStorage storage("mongodb://localhost:27017", "test-search-engine");
        auto result = storage.testConnection();
        
        if (result.success) {
            REQUIRE(result.value == true);
            REQUIRE(result.message == "MongoDB connection is healthy");
        } else {
            // If MongoDB is not available, skip the rest of the tests
            WARN("MongoDB not available: " + result.message);
            return;
        }
    }
}

TEST_CASE("MongoDB Storage - Site Profile CRUD Operations", "[mongodb][storage][crud]") {
    MongoDBStorage storage("mongodb://localhost:27017", "test-search-engine");
    
    // Skip tests if MongoDB is not available
    auto connectionTest = storage.testConnection();
    if (!connectionTest.success) {
        WARN("Skipping MongoDB tests - MongoDB not available");
        return;
    }
    
    SECTION("Store and retrieve site profile") {
        SiteProfile testProfile = createTestSiteProfile("https://hatef.ir");
        
        // Store the profile
        auto storeResult = storage.storeSiteProfile(testProfile);
        REQUIRE(storeResult.success);
        REQUIRE(!storeResult.value.empty());
        
        std::string profileId = storeResult.value;
        
        // Retrieve by URL
        auto retrieveResult = storage.getSiteProfile("https://hatef.ir");
        REQUIRE(retrieveResult.success);
        
        SiteProfile retrieved = retrieveResult.value;
        REQUIRE(retrieved.url == testProfile.url);
        REQUIRE(retrieved.domain == testProfile.domain);
        REQUIRE(retrieved.title == testProfile.title);
        REQUIRE(retrieved.description == testProfile.description);
        REQUIRE(retrieved.keywords == testProfile.keywords);
        REQUIRE(retrieved.language == testProfile.language);
        REQUIRE(retrieved.category == testProfile.category);
        
        // Retrieve by ID
        auto retrieveByIdResult = storage.getSiteProfileById(profileId);
        REQUIRE(retrieveByIdResult.success);
        REQUIRE(retrieveByIdResult.value.url == testProfile.url);
        
        // Clean up
        storage.deleteSiteProfile("https://hatef.ir");
    }
    
    SECTION("Update site profile") {
        SiteProfile testProfile = createTestSiteProfile("https://hatef.ir");
        
        // Store the profile
        auto storeResult = storage.storeSiteProfile(testProfile);
        REQUIRE(storeResult.success);
        
        // Retrieve and modify
        auto retrieveResult = storage.getSiteProfile("https://hatef.ir");
        REQUIRE(retrieveResult.success);
        
        SiteProfile retrieved = retrieveResult.value;
        retrieved.title = "Updated Title";
        retrieved.crawlMetadata.crawlCount = 2;
        retrieved.contentQuality = 0.9;
        
        // Update
        auto updateResult = storage.updateSiteProfile(retrieved);
        REQUIRE(updateResult.success);
        
        // Retrieve again and verify changes
        auto verifyResult = storage.getSiteProfile("https://hatef.ir");
        REQUIRE(verifyResult.success);
        
        SiteProfile verified = verifyResult.value;
        REQUIRE(verified.title == "Updated Title");
        REQUIRE(verified.crawlMetadata.crawlCount == 2);
        REQUIRE(verified.contentQuality == 0.9);
        
        // Clean up
        storage.deleteSiteProfile("https://hatef.ir");
    }
    
    SECTION("Delete site profile") {
        SiteProfile testProfile = createTestSiteProfile("https://test-delete.com");
        
        // Store the profile
        auto storeResult = storage.storeSiteProfile(testProfile);
        REQUIRE(storeResult.success);
        
        // Verify it exists
        auto retrieveResult = storage.getSiteProfile("https://test-delete.com");
        REQUIRE(retrieveResult.success);
        
        // Delete
        auto deleteResult = storage.deleteSiteProfile("https://test-delete.com");
        REQUIRE(deleteResult.success);
        
        // Verify it's gone
        auto verifyResult = storage.getSiteProfile("https://test-delete.com");
        REQUIRE(!verifyResult.success);
    }
    
    SECTION("Non-existent profile retrieval") {
        auto result = storage.getSiteProfile("https://non-existent.com");
        REQUIRE(!result.success);
        REQUIRE(result.message.find("not found") != std::string::npos);
    }
} 