#include "../../include/search_engine/storage/MongoDBStorage.h"
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <sstream>
#include <iomanip>
#include <mutex>

using namespace bsoncxx::builder::stream;
using namespace search_engine::storage;

namespace {
    // Singleton for mongocxx instance
    class MongoInstance {
    private:
        static std::unique_ptr<mongocxx::instance> instance_;
        static std::mutex mutex_;
    public:
        static mongocxx::instance& getInstance() {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!instance_) {
                instance_ = std::make_unique<mongocxx::instance>();
            }
            return *instance_;
        }
    };
    
    std::unique_ptr<mongocxx::instance> MongoInstance::instance_;
    std::mutex MongoInstance::mutex_;
    
    // Helper function to convert time_point to BSON date
    bsoncxx::types::b_date timePointToBsonDate(const std::chrono::system_clock::time_point& tp) {
        auto duration = tp.time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        return bsoncxx::types::b_date{std::chrono::milliseconds{millis}};
    }
    
    // Helper function to convert BSON date to time_point
    std::chrono::system_clock::time_point bsonDateToTimePoint(const bsoncxx::types::b_date& date) {
        return std::chrono::system_clock::time_point{date.value};
    }
}

MongoDBStorage::MongoDBStorage(const std::string& connectionString, const std::string& databaseName) {
    try {
        // Ensure instance is initialized
        MongoInstance::getInstance();
        
        // Create client and connect to database
        mongocxx::uri uri{connectionString};
        client_ = std::make_unique<mongocxx::client>(uri);
        database_ = (*client_)[databaseName];
        siteProfilesCollection_ = database_["site_profiles"];
        
        // Ensure indexes are created
        ensureIndexes();
        
    } catch (const mongocxx::exception& e) {
        throw std::runtime_error("Failed to initialize MongoDB connection: " + std::string(e.what()));
    }
}

std::string MongoDBStorage::crawlStatusToString(CrawlStatus status) {
    switch (status) {
        case CrawlStatus::SUCCESS: return "SUCCESS";
        case CrawlStatus::FAILED: return "FAILED";
        case CrawlStatus::PENDING: return "PENDING";
        case CrawlStatus::TIMEOUT: return "TIMEOUT";
        case CrawlStatus::ROBOT_BLOCKED: return "ROBOT_BLOCKED";
        case CrawlStatus::REDIRECT_LOOP: return "REDIRECT_LOOP";
        case CrawlStatus::CONTENT_TOO_LARGE: return "CONTENT_TOO_LARGE";
        case CrawlStatus::INVALID_CONTENT_TYPE: return "INVALID_CONTENT_TYPE";
        default: return "UNKNOWN";
    }
}

CrawlStatus MongoDBStorage::stringToCrawlStatus(const std::string& status) {
    if (status == "SUCCESS") return CrawlStatus::SUCCESS;
    if (status == "FAILED") return CrawlStatus::FAILED;
    if (status == "PENDING") return CrawlStatus::PENDING;
    if (status == "TIMEOUT") return CrawlStatus::TIMEOUT;
    if (status == "ROBOT_BLOCKED") return CrawlStatus::ROBOT_BLOCKED;
    if (status == "REDIRECT_LOOP") return CrawlStatus::REDIRECT_LOOP;
    if (status == "CONTENT_TOO_LARGE") return CrawlStatus::CONTENT_TOO_LARGE;
    if (status == "INVALID_CONTENT_TYPE") return CrawlStatus::INVALID_CONTENT_TYPE;
    return CrawlStatus::FAILED;
}

bsoncxx::document::value MongoDBStorage::crawlMetadataToBson(const CrawlMetadata& metadata) const {
    auto builder = document{};
    
    builder << "lastCrawlTime" << timePointToBsonDate(metadata.lastCrawlTime)
            << "firstCrawlTime" << timePointToBsonDate(metadata.firstCrawlTime)
            << "lastCrawlStatus" << crawlStatusToString(metadata.lastCrawlStatus)
            << "crawlCount" << metadata.crawlCount
            << "crawlIntervalHours" << metadata.crawlIntervalHours
            << "userAgent" << metadata.userAgent
            << "httpStatusCode" << metadata.httpStatusCode
            << "contentSize" << static_cast<int64_t>(metadata.contentSize)
            << "contentType" << metadata.contentType
            << "crawlDurationMs" << metadata.crawlDurationMs;
    
    if (metadata.lastErrorMessage) {
        builder << "lastErrorMessage" << *metadata.lastErrorMessage;
    }
    
    return builder << finalize;
}

CrawlMetadata MongoDBStorage::bsonToCrawlMetadata(const bsoncxx::document::view& doc) const {
    CrawlMetadata metadata;
    
    metadata.lastCrawlTime = bsonDateToTimePoint(doc["lastCrawlTime"].get_date());
    metadata.firstCrawlTime = bsonDateToTimePoint(doc["firstCrawlTime"].get_date());
    metadata.lastCrawlStatus = stringToCrawlStatus(std::string(doc["lastCrawlStatus"].get_string().value));
    metadata.crawlCount = doc["crawlCount"].get_int32().value;
    metadata.crawlIntervalHours = doc["crawlIntervalHours"].get_double().value;
    metadata.userAgent = std::string(doc["userAgent"].get_string().value);
    metadata.httpStatusCode = doc["httpStatusCode"].get_int32().value;
    metadata.contentSize = static_cast<size_t>(doc["contentSize"].get_int64().value);
    metadata.contentType = std::string(doc["contentType"].get_string().value);
    metadata.crawlDurationMs = doc["crawlDurationMs"].get_double().value;
    
    if (doc["lastErrorMessage"]) {
        metadata.lastErrorMessage = std::string(doc["lastErrorMessage"].get_string().value);
    }
    
    return metadata;
}

bsoncxx::document::value MongoDBStorage::siteProfileToBson(const SiteProfile& profile) const {
    auto builder = document{};
    
    if (profile.id) {
        builder << "_id" << bsoncxx::oid{*profile.id};
    }
    
    builder << "domain" << profile.domain
            << "url" << profile.url
            << "title" << profile.title
            << "isIndexed" << profile.isIndexed
            << "lastModified" << timePointToBsonDate(profile.lastModified)
            << "indexedAt" << timePointToBsonDate(profile.indexedAt);
    
    // Optional fields
    if (profile.description) {
        builder << "description" << *profile.description;
    }
    if (profile.language) {
        builder << "language" << *profile.language;
    }
    if (profile.category) {
        builder << "category" << *profile.category;
    }
    if (profile.pageRank) {
        builder << "pageRank" << *profile.pageRank;
    }
    if (profile.contentQuality) {
        builder << "contentQuality" << *profile.contentQuality;
    }
    if (profile.wordCount) {
        builder << "wordCount" << *profile.wordCount;
    }
    if (profile.isMobile) {
        builder << "isMobile" << *profile.isMobile;
    }
    if (profile.hasSSL) {
        builder << "hasSSL" << *profile.hasSSL;
    }
    if (profile.inboundLinkCount) {
        builder << "inboundLinkCount" << *profile.inboundLinkCount;
    }
    if (profile.author) {
        builder << "author" << *profile.author;
    }
    if (profile.publisher) {
        builder << "publisher" << *profile.publisher;
    }
    if (profile.publishDate) {
        builder << "publishDate" << timePointToBsonDate(*profile.publishDate);
    }
    
    // Arrays
    auto keywordsArray = array{};
    for (const auto& keyword : profile.keywords) {
        keywordsArray << keyword;
    }
    builder << "keywords" << keywordsArray;
    
    auto outboundLinksArray = array{};
    for (const auto& link : profile.outboundLinks) {
        outboundLinksArray << link;
    }
    builder << "outboundLinks" << outboundLinksArray;
    
    // Crawl metadata
    builder << "crawlMetadata" << crawlMetadataToBson(profile.crawlMetadata);
    
    return builder << finalize;
}

SiteProfile MongoDBStorage::bsonToSiteProfile(const bsoncxx::document::view& doc) const {
    SiteProfile profile;
    
    if (doc["_id"]) {
        profile.id = std::string(doc["_id"].get_oid().value.to_string());
    }
    
    profile.domain = std::string(doc["domain"].get_string().value);
    profile.url = std::string(doc["url"].get_string().value);
    profile.title = std::string(doc["title"].get_string().value);
    profile.isIndexed = doc["isIndexed"].get_bool().value;
    profile.lastModified = bsonDateToTimePoint(doc["lastModified"].get_date());
    profile.indexedAt = bsonDateToTimePoint(doc["indexedAt"].get_date());
    
    // Optional fields
    if (doc["description"]) {
        profile.description = std::string(doc["description"].get_string().value);
    }
    if (doc["language"]) {
        profile.language = std::string(doc["language"].get_string().value);
    }
    if (doc["category"]) {
        profile.category = std::string(doc["category"].get_string().value);
    }
    if (doc["pageRank"]) {
        profile.pageRank = doc["pageRank"].get_int32().value;
    }
    if (doc["contentQuality"]) {
        profile.contentQuality = doc["contentQuality"].get_double().value;
    }
    if (doc["wordCount"]) {
        profile.wordCount = doc["wordCount"].get_int32().value;
    }
    if (doc["isMobile"]) {
        profile.isMobile = doc["isMobile"].get_bool().value;
    }
    if (doc["hasSSL"]) {
        profile.hasSSL = doc["hasSSL"].get_bool().value;
    }
    if (doc["inboundLinkCount"]) {
        profile.inboundLinkCount = doc["inboundLinkCount"].get_int32().value;
    }
    if (doc["author"]) {
        profile.author = std::string(doc["author"].get_string().value);
    }
    if (doc["publisher"]) {
        profile.publisher = std::string(doc["publisher"].get_string().value);
    }
    if (doc["publishDate"]) {
        profile.publishDate = bsonDateToTimePoint(doc["publishDate"].get_date());
    }
    
    // Arrays
    if (doc["keywords"]) {
        for (const auto& keyword : doc["keywords"].get_array().value) {
            profile.keywords.push_back(std::string(keyword.get_string().value));
        }
    }
    
    if (doc["outboundLinks"]) {
        for (const auto& link : doc["outboundLinks"].get_array().value) {
            profile.outboundLinks.push_back(std::string(link.get_string().value));
        }
    }
    
    // Crawl metadata
    if (doc["crawlMetadata"]) {
        profile.crawlMetadata = bsonToCrawlMetadata(doc["crawlMetadata"].get_document().view());
    }
    
    return profile;
}

Result<std::string> MongoDBStorage::storeSiteProfile(const SiteProfile& profile) {
    try {
        auto doc = siteProfileToBson(profile);
        auto result = siteProfilesCollection_.insert_one(doc.view());
        
        if (result) {
            return Result<std::string>::Success(
                std::string(result->inserted_id().get_oid().value.to_string()),
                "Site profile stored successfully"
            );
        } else {
            return Result<std::string>::Failure("Failed to insert site profile");
        }
    } catch (const mongocxx::exception& e) {
        return Result<std::string>::Failure("MongoDB error: " + std::string(e.what()));
    }
}

Result<SiteProfile> MongoDBStorage::getSiteProfile(const std::string& url) {
    try {
        auto filter = document{} << "url" << url << finalize;
        auto result = siteProfilesCollection_.find_one(filter.view());
        
        if (result) {
            return Result<SiteProfile>::Success(
                bsonToSiteProfile(result->view()),
                "Site profile retrieved successfully"
            );
        } else {
            return Result<SiteProfile>::Failure("Site profile not found for URL: " + url);
        }
    } catch (const mongocxx::exception& e) {
        return Result<SiteProfile>::Failure("MongoDB error: " + std::string(e.what()));
    }
}

Result<SiteProfile> MongoDBStorage::getSiteProfileById(const std::string& id) {
    try {
        auto filter = document{} << "_id" << bsoncxx::oid{id} << finalize;
        auto result = siteProfilesCollection_.find_one(filter.view());
        
        if (result) {
            return Result<SiteProfile>::Success(
                bsonToSiteProfile(result->view()),
                "Site profile retrieved successfully"
            );
        } else {
            return Result<SiteProfile>::Failure("Site profile not found for ID: " + id);
        }
    } catch (const mongocxx::exception& e) {
        return Result<SiteProfile>::Failure("MongoDB error: " + std::string(e.what()));
    }
}

Result<bool> MongoDBStorage::updateSiteProfile(const SiteProfile& profile) {
    try {
        if (!profile.id) {
            return Result<bool>::Failure("Cannot update site profile without ID");
        }
        
        auto filter = document{} << "_id" << bsoncxx::oid{*profile.id} << finalize;
        auto update = document{} << "$set" << siteProfileToBson(profile) << finalize;
        
        auto result = siteProfilesCollection_.update_one(filter.view(), update.view());
        
        if (result && result->modified_count() > 0) {
            return Result<bool>::Success(true, "Site profile updated successfully");
        } else {
            return Result<bool>::Failure("Site profile not found or no changes made");
        }
    } catch (const mongocxx::exception& e) {
        return Result<bool>::Failure("MongoDB error: " + std::string(e.what()));
    }
}

Result<bool> MongoDBStorage::deleteSiteProfile(const std::string& url) {
    try {
        auto filter = document{} << "url" << url << finalize;
        auto result = siteProfilesCollection_.delete_one(filter.view());
        
        if (result && result->deleted_count() > 0) {
            return Result<bool>::Success(true, "Site profile deleted successfully");
        } else {
            return Result<bool>::Failure("Site profile not found for URL: " + url);
        }
    } catch (const mongocxx::exception& e) {
        return Result<bool>::Failure("MongoDB error: " + std::string(e.what()));
    }
}

Result<std::vector<SiteProfile>> MongoDBStorage::getSiteProfilesByDomain(const std::string& domain) {
    try {
        auto filter = document{} << "domain" << domain << finalize;
        auto cursor = siteProfilesCollection_.find(filter.view());
        
        std::vector<SiteProfile> profiles;
        for (const auto& doc : cursor) {
            profiles.push_back(bsonToSiteProfile(doc));
        }
        
        return Result<std::vector<SiteProfile>>::Success(
            std::move(profiles),
            "Site profiles retrieved successfully for domain: " + domain
        );
    } catch (const mongocxx::exception& e) {
        return Result<std::vector<SiteProfile>>::Failure("MongoDB error: " + std::string(e.what()));
    }
}

Result<std::vector<SiteProfile>> MongoDBStorage::getSiteProfilesByCrawlStatus(CrawlStatus status) {
    try {
        auto filter = document{} << "crawlMetadata.lastCrawlStatus" << crawlStatusToString(status) << finalize;
        auto cursor = siteProfilesCollection_.find(filter.view());
        
        std::vector<SiteProfile> profiles;
        for (const auto& doc : cursor) {
            profiles.push_back(bsonToSiteProfile(doc));
        }
        
        return Result<std::vector<SiteProfile>>::Success(
            std::move(profiles),
            "Site profiles retrieved successfully for status"
        );
    } catch (const mongocxx::exception& e) {
        return Result<std::vector<SiteProfile>>::Failure("MongoDB error: " + std::string(e.what()));
    }
}

Result<int64_t> MongoDBStorage::getTotalSiteCount() {
    try {
        auto count = siteProfilesCollection_.count_documents({});
        return Result<int64_t>::Success(count, "Site count retrieved successfully");
    } catch (const mongocxx::exception& e) {
        return Result<int64_t>::Failure("MongoDB error: " + std::string(e.what()));
    }
}

Result<int64_t> MongoDBStorage::getSiteCountByStatus(CrawlStatus status) {
    try {
        auto filter = document{} << "crawlMetadata.lastCrawlStatus" << crawlStatusToString(status) << finalize;
        auto count = siteProfilesCollection_.count_documents(filter.view());
        return Result<int64_t>::Success(count, "Site count by status retrieved successfully");
    } catch (const mongocxx::exception& e) {
        return Result<int64_t>::Failure("MongoDB error: " + std::string(e.what()));
    }
}

Result<bool> MongoDBStorage::testConnection() {
    try {
        // Simple ping to test connection
        auto result = database_.run_command(document{} << "ping" << 1 << finalize);
        return Result<bool>::Success(true, "MongoDB connection is healthy");
    } catch (const mongocxx::exception& e) {
        return Result<bool>::Failure("MongoDB connection test failed: " + std::string(e.what()));
    }
}

Result<bool> MongoDBStorage::ensureIndexes() {
    try {
        // Create indexes for efficient querying
        auto urlIndex = document{} << "url" << 1 << finalize;
        auto domainIndex = document{} << "domain" << 1 << finalize;
        auto statusIndex = document{} << "crawlMetadata.lastCrawlStatus" << 1 << finalize;
        auto lastModifiedIndex = document{} << "lastModified" << -1 << finalize;
        
        siteProfilesCollection_.create_index(urlIndex.view());
        siteProfilesCollection_.create_index(domainIndex.view());
        siteProfilesCollection_.create_index(statusIndex.view());
        siteProfilesCollection_.create_index(lastModifiedIndex.view());
        
        return Result<bool>::Success(true, "Indexes created successfully");
    } catch (const mongocxx::exception& e) {
        return Result<bool>::Failure("Failed to create indexes: " + std::string(e.what()));
    }
} 