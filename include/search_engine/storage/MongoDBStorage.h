#pragma once

#include "SiteProfile.h"
#include "../../infrastructure.h"
#include <mongocxx/client.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/instance.hpp>
#include <bsoncxx/document/value.hpp>
#include <bsoncxx/document/view.hpp>
#include <memory>
#include <vector>
#include <optional>

namespace search_engine {
namespace storage {

class MongoDBStorage {
private:
    std::unique_ptr<mongocxx::client> client_;
    mongocxx::database database_;
    mongocxx::collection siteProfilesCollection_;
    
    // Conversion methods between SiteProfile and BSON
    bsoncxx::document::value siteProfileToBson(const SiteProfile& profile) const;
    SiteProfile bsonToSiteProfile(const bsoncxx::document::view& doc) const;
    
    // Helper methods for BSON conversion
    bsoncxx::document::value crawlMetadataToBson(const CrawlMetadata& metadata) const;
    CrawlMetadata bsonToCrawlMetadata(const bsoncxx::document::view& doc) const;
    
    static std::string crawlStatusToString(CrawlStatus status);
    static CrawlStatus stringToCrawlStatus(const std::string& status);

public:
    // Constructor with connection string
    explicit MongoDBStorage(const std::string& connectionString = "mongodb://localhost:27017",
                           const std::string& databaseName = "search-engine");
    
    // Destructor
    ~MongoDBStorage() = default;
    
    // Move constructor and assignment (delete copy operations for RAII)
    MongoDBStorage(MongoDBStorage&& other) noexcept = default;
    MongoDBStorage& operator=(MongoDBStorage&& other) noexcept = default;
    MongoDBStorage(const MongoDBStorage&) = delete;
    MongoDBStorage& operator=(const MongoDBStorage&) = delete;
    
    // Core storage operations
    Result<std::string> storeSiteProfile(const SiteProfile& profile);
    Result<SiteProfile> getSiteProfile(const std::string& url);
    Result<SiteProfile> getSiteProfileById(const std::string& id);
    Result<bool> updateSiteProfile(const SiteProfile& profile);
    Result<bool> deleteSiteProfile(const std::string& url);
    
    // Batch operations
    Result<std::vector<std::string>> storeSiteProfiles(const std::vector<SiteProfile>& profiles);
    Result<std::vector<SiteProfile>> getSiteProfilesByDomain(const std::string& domain);
    Result<std::vector<SiteProfile>> getSiteProfilesByCrawlStatus(CrawlStatus status);
    
    // Search and filtering
    Result<std::vector<SiteProfile>> searchSiteProfiles(
        const std::string& query,
        int limit = 100,
        int skip = 0
    );
    
    // Statistics and maintenance
    Result<int64_t> getTotalSiteCount();
    Result<int64_t> getSiteCountByStatus(CrawlStatus status);
    Result<bool> createIndexes();
    
    // Connection management
    Result<bool> testConnection();
    Result<bool> ensureIndexes();
};

} // namespace storage
} // namespace search_engine 