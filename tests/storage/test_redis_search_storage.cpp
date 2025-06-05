#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include "../../include/search_engine/storage/RedisSearchStorage.h"
#include <chrono>
#include <thread>

using namespace search_engine::storage;

// Test data helpers
namespace {
    SearchDocument createTestSearchDocument(const std::string& url = "https://example.com") {
        SearchDocument doc;
        doc.url = url;
        doc.title = "Test Document";
        doc.content = "This is a test document with some sample content for testing full-text search capabilities.";
        doc.domain = "example.com";
        doc.keywords = {"test", "document", "search", "example"};
        doc.description = "A sample document for testing";
        doc.language = "en";
        doc.category = "test";
        doc.indexedAt = std::chrono::system_clock::now();
        doc.score = 0.8;
        
        return doc;
    }
    
    SiteProfile createTestSiteProfile(const std::string& url = "https://example.com") {
        SiteProfile profile;
        profile.domain = "example.com";
        profile.url = url;
        profile.title = "Test Site";
        profile.description = "A test website for unit testing";
        profile.keywords = {"test", "example", "website"};
        profile.language = "en";
        profile.category = "technology";
        
        // Set required timestamps
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
        
        profile.isIndexed = true;
        profile.lastModified = now;
        profile.indexedAt = now;
        profile.contentQuality = 0.8;
        
        return profile;
    }
}

TEST_CASE("RedisSearch Storage - Connection and Initialization", "[redis][storage]") {
    SECTION("Constructor with default parameters") {
        REQUIRE_NOTHROW(RedisSearchStorage storage);
    }
    
    SECTION("Constructor with custom parameters") {
        REQUIRE_NOTHROW(RedisSearchStorage storage("tcp://127.0.0.1:6379", "test_index", "test:"));
    }
    
    SECTION("Test connection") {
        RedisSearchStorage storage("tcp://127.0.0.1:6379", "test_index");
        auto result = storage.testConnection();
        
        if (result.isSuccess()) {
            REQUIRE(result.getValue() == true);
            REQUIRE(result.getMessage() == "Redis connection is healthy");
        } else {
            // If Redis is not available, skip the rest of the tests
            WARN("Redis not available: " + result.getErrorMessage());
            return;
        }
    }
    
    SECTION("Initialize index") {
        RedisSearchStorage storage("tcp://127.0.0.1:6379", "test_init_index");
        auto connectionTest = storage.testConnection();
        
        if (!connectionTest.isSuccess()) {
            WARN("Skipping Redis tests - Redis not available");
            return;
        }
        
        // Drop index first (ignore if fails)
        storage.dropIndex();
        
        auto result = storage.initializeIndex();
        REQUIRE(result.isSuccess());
        
        // Clean up
        storage.dropIndex();
    }
}

TEST_CASE("RedisSearch Storage - Document Indexing and Retrieval", "[redis][storage][indexing]") {
    RedisSearchStorage storage("tcp://127.0.0.1:6379", "test_doc_index");
    
    // Skip tests if Redis is not available
    auto connectionTest = storage.testConnection();
    if (!connectionTest.isSuccess()) {
        WARN("Skipping Redis tests - Redis not available");
        return;
    }
    
    // Initialize clean index
    storage.dropIndex();
    auto initResult = storage.initializeIndex();
    REQUIRE(initResult.isSuccess());
    
    SECTION("Index and search single document") {
        SearchDocument testDoc = createTestSearchDocument("https://test-doc.com");
        testDoc.title = "Unique Test Document";
        testDoc.content = "This document contains unique content for testing search functionality.";
        
        // Index the document
        auto indexResult = storage.indexDocument(testDoc);
        REQUIRE(indexResult.isSuccess());
        
        // Give Redis a moment to process
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Search for the document
        auto searchResult = storage.searchSimple("unique test", 10);
        REQUIRE(searchResult.isSuccess());
        
        auto response = searchResult.getValue();
        REQUIRE(response.results.size() >= 1);
        
        // Verify the result
        bool found = false;
        for (const auto& result : response.results) {
            if (result.url == "https://test-doc.com") {
                REQUIRE(result.title == "Unique Test Document");
                REQUIRE(!result.snippet.empty());
                REQUIRE(result.domain == "test-doc.com");
                found = true;
                break;
            }
        }
        REQUIRE(found);
        
        // Clean up
        storage.deleteDocument("https://test-doc.com");
    }
    
    SECTION("Index site profile") {
        SiteProfile testProfile = createTestSiteProfile("https://profile-test.com");
        testProfile.title = "Profile Test Site";
        
        std::string content = "This is the main content of the profile test site with searchable text.";
        
        // Index the site profile
        auto indexResult = storage.indexSiteProfile(testProfile, content);
        REQUIRE(indexResult.isSuccess());
        
        // Give Redis a moment to process
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Search for the profile
        auto searchResult = storage.searchSimple("profile test", 10);
        REQUIRE(searchResult.isSuccess());
        
        auto response = searchResult.getValue();
        REQUIRE(response.results.size() >= 1);
        
        // Clean up
        storage.deleteDocument("https://profile-test.com");
    }
    
    SECTION("Update document") {
        SearchDocument testDoc = createTestSearchDocument("https://update-test.com");
        testDoc.title = "Original Title";
        
        // Index original document
        auto indexResult = storage.indexDocument(testDoc);
        REQUIRE(indexResult.isSuccess());
        
        // Update the document
        testDoc.title = "Updated Title";
        testDoc.content = "Updated content for testing document updates.";
        
        auto updateResult = storage.updateDocument(testDoc);
        REQUIRE(updateResult.isSuccess());
        
        // Give Redis a moment to process
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Search for the updated document
        auto searchResult = storage.searchSimple("updated", 10);
        REQUIRE(searchResult.isSuccess());
        
        auto response = searchResult.getValue();
        REQUIRE(response.results.size() >= 1);
        
        // Clean up
        storage.deleteDocument("https://update-test.com");
    }
    
    SECTION("Delete document") {
        SearchDocument testDoc = createTestSearchDocument("https://delete-test.com");
        testDoc.title = "Document to Delete";
        
        // Index the document
        auto indexResult = storage.indexDocument(testDoc);
        REQUIRE(indexResult.isSuccess());
        
        // Give Redis a moment to process
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Verify it's searchable
        auto searchResult = storage.searchSimple("delete", 10);
        REQUIRE(searchResult.isSuccess());
        REQUIRE(searchResult.getValue().results.size() >= 1);
        
        // Delete the document
        auto deleteResult = storage.deleteDocument("https://delete-test.com");
        REQUIRE(deleteResult.isSuccess());
        
        // Give Redis a moment to process
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Verify it's no longer searchable
        auto searchAfterDelete = storage.searchSimple("delete document", 10);
        if (searchAfterDelete.isSuccess()) {
            bool found = false;
            for (const auto& result : searchAfterDelete.getValue().results) {
                if (result.url == "https://delete-test.com") {
                    found = true;
                    break;
                }
            }
            REQUIRE(!found);
        }
    }
    
    // Clean up the test index
    storage.dropIndex();
}

TEST_CASE("RedisSearch Storage - Advanced Search Features", "[redis][storage][search]") {
    RedisSearchStorage storage("tcp://127.0.0.1:6379", "test_advanced_index");
    
    // Skip tests if Redis is not available
    auto connectionTest = storage.testConnection();
    if (!connectionTest.isSuccess()) {
        WARN("Skipping Redis tests - Redis not available");
        return;
    }
    
    // Initialize clean index
    storage.dropIndex();
    auto initResult = storage.initializeIndex();
    REQUIRE(initResult.isSuccess());
    
    // Index multiple test documents
    std::vector<SearchDocument> testDocs = {
        createTestSearchDocument("https://tech.com"),
        createTestSearchDocument("https://science.com"),
        createTestSearchDocument("https://news.com")
    };
    
    testDocs[0].title = "Technology News";
    testDocs[0].content = "Latest technology trends and innovations in software development.";
    testDocs[0].category = "technology";
    testDocs[0].domain = "tech.com";
    
    testDocs[1].title = "Science Discovery";
    testDocs[1].content = "New scientific discoveries in physics and chemistry research.";
    testDocs[1].category = "science";
    testDocs[1].domain = "science.com";
    
    testDocs[2].title = "Breaking News";
    testDocs[2].content = "Latest news updates from around the world covering various topics.";
    testDocs[2].category = "news";
    testDocs[2].domain = "news.com";
    
    // Index all documents
    for (const auto& doc : testDocs) {
        auto result = storage.indexDocument(doc);
        REQUIRE(result.isSuccess());
    }
    
    // Give Redis a moment to process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    SECTION("Advanced search with filters") {
        SearchQuery query;
        query.query = "news";
        query.limit = 10;
        query.category = "technology";
        
        auto searchResult = storage.search(query);
        REQUIRE(searchResult.isSuccess());
        
        auto response = searchResult.getValue();
        // Should find technology news
        REQUIRE(response.results.size() >= 1);
    }
    
    SECTION("Search with highlighting") {
        SearchQuery query;
        query.query = "technology";
        query.limit = 10;
        query.highlight = true;
        
        auto searchResult = storage.search(query);
        REQUIRE(searchResult.isSuccess());
        
        auto response = searchResult.getValue();
        REQUIRE(response.results.size() >= 1);
        REQUIRE(response.queryTime >= 0);
    }
    
    SECTION("Get document count") {
        auto countResult = storage.getDocumentCount();
        REQUIRE(countResult.isSuccess());
        REQUIRE(countResult.getValue() >= 3);
    }
    
    SECTION("Get index info") {
        auto infoResult = storage.getIndexInfo();
        REQUIRE(infoResult.isSuccess());
        
        auto info = infoResult.getValue();
        REQUIRE(!info.empty());
    }
    
    // Clean up the test index
    storage.dropIndex();
}

TEST_CASE("RedisSearch Storage - Batch Operations", "[redis][storage][batch]") {
    RedisSearchStorage storage("tcp://127.0.0.1:6379", "test_batch_index");
    
    // Skip tests if Redis is not available
    auto connectionTest = storage.testConnection();
    if (!connectionTest.isSuccess()) {
        WARN("Skipping Redis tests - Redis not available");
        return;
    }
    
    // Initialize clean index
    storage.dropIndex();
    auto initResult = storage.initializeIndex();
    REQUIRE(initResult.isSuccess());
    
    SECTION("Index multiple documents") {
        std::vector<SearchDocument> testDocs;
        for (int i = 0; i < 5; ++i) {
            SearchDocument doc = createTestSearchDocument("https://batch" + std::to_string(i) + ".com");
            doc.title = "Batch Document " + std::to_string(i);
            doc.content = "Content for batch document number " + std::to_string(i) + " with unique text.";
            doc.domain = "batch" + std::to_string(i) + ".com";
            testDocs.push_back(doc);
        }
        
        // Index documents in batch
        auto batchResult = storage.indexDocuments(testDocs);
        REQUIRE(batchResult.isSuccess());
        
        auto results = batchResult.getValue();
        REQUIRE(results.size() == 5);
        
        // Verify all succeeded
        for (bool success : results) {
            REQUIRE(success);
        }
        
        // Give Redis a moment to process
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Verify documents are searchable
        auto searchResult = storage.searchSimple("batch document", 10);
        REQUIRE(searchResult.isSuccess());
        REQUIRE(searchResult.getValue().results.size() >= 5);
        
        // Clean up
        for (int i = 0; i < 5; ++i) {
            storage.deleteDocument("https://batch" + std::to_string(i) + ".com");
        }
    }
    
    // Clean up the test index
    storage.dropIndex();
}

TEST_CASE("RedisSearch Storage - Error Handling", "[redis][storage][errors]") {
    SECTION("Delete non-existent document") {
        RedisSearchStorage storage("tcp://127.0.0.1:6379", "test_error_index");
        
        auto connectionTest = storage.testConnection();
        if (!connectionTest.isSuccess()) {
            WARN("Skipping Redis tests - Redis not available");
            return;
        }
        
        auto deleteResult = storage.deleteDocument("https://non-existent.com");
        REQUIRE(!deleteResult.isSuccess());
        REQUIRE(deleteResult.getErrorMessage().find("not found") != std::string::npos);
    }
    
    SECTION("Search on non-existent index") {
        RedisSearchStorage storage("tcp://127.0.0.1:6379", "non_existent_index");
        
        auto connectionTest = storage.testConnection();
        if (!connectionTest.isSuccess()) {
            WARN("Skipping Redis tests - Redis not available");
            return;
        }
        
        auto searchResult = storage.searchSimple("test", 10);
        // This might succeed with 0 results or fail - either is acceptable
        // The important thing is that it doesn't crash
        REQUIRE_NOTHROW(storage.searchSimple("test", 10));
    }
}

TEST_CASE("RedisSearch Storage - Utility Functions", "[redis][storage][utils]") {
    SECTION("SiteProfile to SearchDocument conversion") {
        SiteProfile profile = createTestSiteProfile("https://convert-test.com");
        profile.title = "Conversion Test Site";
        profile.description = "Site for testing conversion";
        
        std::string content = "This is the main content of the site.";
        
        SearchDocument doc = RedisSearchStorage::siteProfileToSearchDocument(profile, content);
        
        REQUIRE(doc.url == profile.url);
        REQUIRE(doc.title == profile.title);
        REQUIRE(doc.content == content);
        REQUIRE(doc.domain == profile.domain);
        REQUIRE(doc.keywords == profile.keywords);
        REQUIRE(doc.description == profile.description);
        REQUIRE(doc.language == profile.language);
        REQUIRE(doc.category == profile.category);
        REQUIRE(doc.score == profile.contentQuality.value_or(0.0));
    }
} 