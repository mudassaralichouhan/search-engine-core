#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <optional>
#include <nlohmann/json.hpp>
#include "../mongodb.h"

using json = nlohmann::json;

enum class DomainStatus {
    PENDING = 0,
    CRAWLING = 1,
    COMPLETED = 2,
    FAILED = 3,
    EMAIL_SENT = 4
};

struct Domain {
    std::string id;
    std::string domain;
    std::string webmasterEmail;
    DomainStatus status;
    int maxPages;
    int pagesCrawled;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point lastCrawledAt;
    std::chrono::system_clock::time_point emailSentAt;
    std::string crawlSessionId;
    std::string crawlJobId;
    std::string emailJobId;
    json metadata;
    std::string errorMessage;
    
    // Business Information
    std::string businessName;
    std::string ownerName;
    std::string phone;
    std::string businessHours;
    std::string address;
    std::string province;
    std::string city;
    
    // Location coordinates
    double latitude;
    double longitude;
    
    // Domain registration dates
    std::string grantDatePersian;
    std::string grantDateGregorian;
    std::string expiryDatePersian;
    std::string expiryDateGregorian;
    
    // Extraction metadata
    std::string extractionTimestamp;
    std::string domainUrl;
    int pageNumber;
    int rowIndex;
    std::string rowNumber;
    
    json toJson() const;
    static Domain fromJson(const json& j);
};

struct DomainBatch {
    std::string id;
    std::string name;
    std::string description;
    std::vector<std::string> domainIds;
    std::chrono::system_clock::time_point createdAt;
    DomainStatus status;
    int totalDomains;
    int completedDomains;
    int failedDomains;
    
    json toJson() const;
    static DomainBatch fromJson(const json& j);
};

class DomainStorage {
public:
    DomainStorage();
    ~DomainStorage() = default;
    
    // Domain management
    std::string addDomain(const std::string& domain, const std::string& webmasterEmail, int maxPages = 5);
    std::vector<std::string> addDomains(const std::vector<std::pair<std::string, std::string>>& domains, int maxPages = 5);
    std::string addDomainWithBusinessInfo(const json& domainData, int maxPages = 5);
    
    bool updateDomain(const Domain& domain);
    bool updateDomainStatus(const std::string& domainId, DomainStatus status);
    bool updateDomainCrawlInfo(const std::string& domainId, const std::string& sessionId, const std::string& jobId, int pagesCrawled = 0);
    bool updateDomainEmailInfo(const std::string& domainId, const std::string& emailJobId);
    
    std::optional<Domain> getDomain(const std::string& domainId);
    std::optional<Domain> getDomainByName(const std::string& domainName);
    std::vector<Domain> getDomainsByStatus(DomainStatus status, int limit = 100, int offset = 0);
    std::vector<Domain> getAllDomains(int limit = 1000, int offset = 0);
    
    bool deleteDomain(const std::string& domainId);
    
    // Batch management
    std::string createBatch(const std::string& name, const std::string& description, const std::vector<std::string>& domainIds);
    bool updateBatch(const DomainBatch& batch);
    std::optional<DomainBatch> getBatch(const std::string& batchId);
    std::vector<DomainBatch> getAllBatches(int limit = 100, int offset = 0);
    bool deleteBatch(const std::string& batchId);
    
    // Statistics
    struct DomainStats {
        int totalDomains;
        int pendingDomains;
        int crawlingDomains;
        int completedDomains;
        int failedDomains;
        int emailsSent;
    };
    DomainStats getStats();
    
    // Bulk operations
    bool importDomainsFromCSV(const std::string& csvContent, int maxPages = 5);
    std::string exportDomainsToCSV(DomainStatus status = static_cast<DomainStatus>(-1));
    
    // Queue management
    std::vector<Domain> getDomainsReadyForCrawling(int limit = 100);
    std::vector<Domain> getDomainsReadyForEmail(int limit = 100);
    
private:
    std::unique_ptr<mongocxx::client> client_;
    mongocxx::database database_;
    mongocxx::collection domainsCollection_;
    mongocxx::collection batchesCollection_;
    
    std::string generateId();
    bsoncxx::document::value domainToBson(const Domain& domain);
    Domain bsonToDomain(const bsoncxx::document::view& doc);
    bsoncxx::document::value batchToBson(const DomainBatch& batch);
    DomainBatch bsonToBatch(const bsoncxx::document::view& doc);
};
