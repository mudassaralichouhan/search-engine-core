#include "../../include/storage/DomainStorage.h"
#include "../../include/Logger.h"
#include "../../include/mongodb.h"
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <uuid/uuid.h>
#include <sstream>
#include <algorithm>

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::array;
using bsoncxx::builder::stream::finalize;

// Domain serialization
json Domain::toJson() const {
    return json{
        {"id", id},
        {"domain", domain},
        {"webmasterEmail", webmasterEmail},
        {"status", static_cast<int>(status)},
        {"maxPages", maxPages},
        {"pagesCrawled", pagesCrawled},
        {"createdAt", std::chrono::duration_cast<std::chrono::seconds>(createdAt.time_since_epoch()).count()},
        {"lastCrawledAt", std::chrono::duration_cast<std::chrono::seconds>(lastCrawledAt.time_since_epoch()).count()},
        {"emailSentAt", std::chrono::duration_cast<std::chrono::seconds>(emailSentAt.time_since_epoch()).count()},
        {"crawlSessionId", crawlSessionId},
        {"crawlJobId", crawlJobId},
        {"emailJobId", emailJobId},
        {"metadata", metadata},
        {"errorMessage", errorMessage},
        {"businessName", businessName},
        {"ownerName", ownerName},
        {"phone", phone},
        {"businessHours", businessHours},
        {"address", address},
        {"province", province},
        {"city", city},
        {"latitude", latitude},
        {"longitude", longitude},
        {"grantDatePersian", grantDatePersian},
        {"grantDateGregorian", grantDateGregorian},
        {"expiryDatePersian", expiryDatePersian},
        {"expiryDateGregorian", expiryDateGregorian},
        {"extractionTimestamp", extractionTimestamp},
        {"domainUrl", domainUrl},
        {"pageNumber", pageNumber},
        {"rowIndex", rowIndex},
        {"rowNumber", rowNumber}
    };
}

Domain Domain::fromJson(const json& j) {
    Domain domain;
    domain.id = j["id"];
    domain.domain = j["domain"];
    domain.webmasterEmail = j["webmasterEmail"];
    domain.status = static_cast<DomainStatus>(j["status"]);
    domain.maxPages = j["maxPages"];
    domain.pagesCrawled = j["pagesCrawled"];
    domain.createdAt = std::chrono::system_clock::from_time_t(j["createdAt"]);
    domain.lastCrawledAt = std::chrono::system_clock::from_time_t(j["lastCrawledAt"]);
    domain.emailSentAt = std::chrono::system_clock::from_time_t(j["emailSentAt"]);
    domain.crawlSessionId = j["crawlSessionId"];
    domain.crawlJobId = j["crawlJobId"];
    domain.emailJobId = j["emailJobId"];
    domain.metadata = j["metadata"];
    domain.errorMessage = j["errorMessage"];
    
    // Business Information
    domain.businessName = j.value("businessName", "");
    domain.ownerName = j.value("ownerName", "");
    domain.phone = j.value("phone", "");
    domain.businessHours = j.value("businessHours", "");
    domain.address = j.value("address", "");
    domain.province = j.value("province", "");
    domain.city = j.value("city", "");
    
    // Location coordinates
    domain.latitude = j.value("latitude", 0.0);
    domain.longitude = j.value("longitude", 0.0);
    
    // Domain registration dates
    domain.grantDatePersian = j.value("grantDatePersian", "");
    domain.grantDateGregorian = j.value("grantDateGregorian", "");
    domain.expiryDatePersian = j.value("expiryDatePersian", "");
    domain.expiryDateGregorian = j.value("expiryDateGregorian", "");
    
    // Extraction metadata
    domain.extractionTimestamp = j.value("extractionTimestamp", "");
    domain.domainUrl = j.value("domainUrl", "");
    domain.pageNumber = j.value("pageNumber", 0);
    domain.rowIndex = j.value("rowIndex", 0);
    domain.rowNumber = j.value("rowNumber", "");
    
    return domain;
}

// DomainBatch serialization
json DomainBatch::toJson() const {
    return json{
        {"id", id},
        {"name", name},
        {"description", description},
        {"domainIds", domainIds},
        {"createdAt", std::chrono::duration_cast<std::chrono::seconds>(createdAt.time_since_epoch()).count()},
        {"status", static_cast<int>(status)},
        {"totalDomains", totalDomains},
        {"completedDomains", completedDomains},
        {"failedDomains", failedDomains}
    };
}

DomainBatch DomainBatch::fromJson(const json& j) {
    DomainBatch batch;
    batch.id = j["id"];
    batch.name = j["name"];
    batch.description = j["description"];
    batch.domainIds = j["domainIds"].get<std::vector<std::string>>();
    batch.createdAt = std::chrono::system_clock::from_time_t(j["createdAt"]);
    batch.status = static_cast<DomainStatus>(j["status"]);
    batch.totalDomains = j["totalDomains"];
    batch.completedDomains = j["completedDomains"];
    batch.failedDomains = j["failedDomains"];
    return batch;
}

DomainStorage::DomainStorage() {
    try {
        LOG_INFO("DomainStorage constructor started");
        
        // Use the singleton instance
        LOG_INFO("Getting MongoDB singleton instance");
        mongocxx::instance& instance = MongoDBInstance::getInstance();
        (void)instance; // Suppress unused variable warning
        LOG_INFO("MongoDB singleton instance obtained");
        
        // Get MongoDB connection string from environment
        const char* mongoUri = std::getenv("MONGODB_URI");
        std::string mongoConnectionString = mongoUri ? mongoUri : "mongodb://admin:password123@mongodb:27017";
        LOG_INFO("MongoDB connection string: " + mongoConnectionString);
        
        LOG_INFO("Creating MongoDB URI");
        mongocxx::uri uri(mongoConnectionString);
        LOG_INFO("MongoDB URI created");
        
        LOG_INFO("Creating MongoDB client");
        client_ = std::make_unique<mongocxx::client>(uri);
        LOG_INFO("MongoDB client created");
        
        LOG_INFO("Getting database and collections");
        database_ = (*client_)["search-engine"];
        domainsCollection_ = database_["domains"];
        batchesCollection_ = database_["domain_batches"];
        LOG_INFO("Database and collections obtained");
        
        // Create indexes for better performance
        LOG_INFO("Creating domain index");
        auto domainIndexDoc = document{}
            << "domain" << 1
            << finalize;
        domainsCollection_.create_index(domainIndexDoc.view());
        LOG_INFO("Domain index created");
        
        LOG_INFO("Creating status index");
        auto statusIndexDoc = document{}
            << "status" << 1
            << finalize;
        domainsCollection_.create_index(statusIndexDoc.view());
        LOG_INFO("Status index created");
        
        LOG_INFO("DomainStorage initialized successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize DomainStorage: " + std::string(e.what()));
        throw;
    } catch (...) {
        LOG_ERROR("Failed to initialize DomainStorage: Unknown exception");
        throw;
    }
}

std::string DomainStorage::addDomain(const std::string& domain, const std::string& webmasterEmail, int maxPages) {
    try {
        Domain domainObj;
        domainObj.id = generateId();
        domainObj.domain = domain;
        domainObj.webmasterEmail = webmasterEmail;
        domainObj.status = DomainStatus::PENDING;
        domainObj.maxPages = maxPages;
        domainObj.pagesCrawled = 0;
        domainObj.createdAt = std::chrono::system_clock::now();
        domainObj.lastCrawledAt = std::chrono::system_clock::time_point{};
        domainObj.emailSentAt = std::chrono::system_clock::time_point{};
        domainObj.metadata = json::object();
        
        // Initialize new business fields with empty values
        domainObj.businessName = "";
        domainObj.ownerName = "";
        domainObj.phone = "";
        domainObj.businessHours = "";
        domainObj.address = "";
        domainObj.province = "";
        domainObj.city = "";
        domainObj.latitude = 0.0;
        domainObj.longitude = 0.0;
        domainObj.grantDatePersian = "";
        domainObj.grantDateGregorian = "";
        domainObj.expiryDatePersian = "";
        domainObj.expiryDateGregorian = "";
        domainObj.extractionTimestamp = "";
        domainObj.domainUrl = "";
        domainObj.pageNumber = 0;
        domainObj.rowIndex = 0;
        domainObj.rowNumber = "";
        
        auto doc = domainToBson(domainObj);
        auto result = domainsCollection_.insert_one(doc.view());
        
        if (result) {
            LOG_INFO("Added domain: " + domain + " with ID: " + domainObj.id);
            return domainObj.id;
        }
        
        LOG_ERROR("Failed to insert domain: " + domain);
        return "";
    } catch (const std::exception& e) {
        LOG_ERROR("Error adding domain " + domain + ": " + std::string(e.what()));
        return "";
    }
}

std::vector<std::string> DomainStorage::addDomains(const std::vector<std::pair<std::string, std::string>>& domains, int maxPages) {
    std::vector<std::string> domainIds;
    std::vector<bsoncxx::document::value> documents;
    
    try {
        for (const auto& [domainName, email] : domains) {
            Domain domainObj;
            domainObj.id = generateId();
            domainObj.domain = domainName;
            domainObj.webmasterEmail = email;
            domainObj.status = DomainStatus::PENDING;
            domainObj.maxPages = maxPages;
            domainObj.pagesCrawled = 0;
            domainObj.createdAt = std::chrono::system_clock::now();
            domainObj.lastCrawledAt = std::chrono::system_clock::time_point{};
            domainObj.emailSentAt = std::chrono::system_clock::time_point{};
            domainObj.metadata = json::object();
            
            // Initialize new business fields with empty values
            domainObj.businessName = "";
            domainObj.ownerName = "";
            domainObj.phone = "";
            domainObj.businessHours = "";
            domainObj.address = "";
            domainObj.province = "";
            domainObj.city = "";
            domainObj.latitude = 0.0;
            domainObj.longitude = 0.0;
            domainObj.grantDatePersian = "";
            domainObj.grantDateGregorian = "";
            domainObj.expiryDatePersian = "";
            domainObj.expiryDateGregorian = "";
            domainObj.extractionTimestamp = "";
            domainObj.domainUrl = "";
            domainObj.pageNumber = 0;
            domainObj.rowIndex = 0;
            domainObj.rowNumber = "";
            
            documents.push_back(domainToBson(domainObj));
            domainIds.push_back(domainObj.id);
        }
        
        auto result = domainsCollection_.insert_many(documents);
        
        if (result && result->inserted_count() == static_cast<std::int32_t>(domains.size())) {
            LOG_INFO("Added " + std::to_string(domains.size()) + " domains successfully");
            return domainIds;
        }
        
        LOG_ERROR("Failed to insert all domains");
        return {};
    } catch (const std::exception& e) {
        LOG_ERROR("Error adding bulk domains: " + std::string(e.what()));
        return {};
    }
}

std::string DomainStorage::addDomainWithBusinessInfo(const json& domainData, int maxPages) {
    try {
        LOG_INFO("Storage Step 1: Starting addDomainWithBusinessInfo");
        Domain domainObj;
        
        LOG_INFO("Storage Step 2: Generating ID");
        domainObj.id = generateId();
        LOG_INFO("Storage Step 2 completed: Generated ID: " + domainObj.id);
        
        LOG_INFO("Storage Step 3: Extracting basic fields");
        domainObj.domain = domainData.value("website_url", "");
        domainObj.webmasterEmail = domainData.value("email", "");
        LOG_INFO("Storage Step 3 completed: domain=" + domainObj.domain + ", email=" + domainObj.webmasterEmail);
        domainObj.status = DomainStatus::PENDING;
        domainObj.maxPages = maxPages;
        domainObj.pagesCrawled = 0;
        domainObj.createdAt = std::chrono::system_clock::now();
        domainObj.lastCrawledAt = std::chrono::system_clock::time_point{};
        domainObj.emailSentAt = std::chrono::system_clock::time_point{};
        domainObj.crawlSessionId = "";
        domainObj.crawlJobId = "";
        domainObj.emailJobId = "";
        domainObj.errorMessage = "";
        domainObj.metadata = json::object();
        
        // Extract business information
        domainObj.businessName = domainData.value("business_name", "");
        domainObj.ownerName = domainData.value("owner_name", "");
        domainObj.phone = domainData.value("phone", "");
        domainObj.businessHours = domainData.value("business_hours", "");
        domainObj.address = domainData.value("address", "");
        
        // Extract location information
        if (domainData.contains("location")) {
            auto location = domainData["location"];
            domainObj.latitude = location.value("latitude", 0.0);
            domainObj.longitude = location.value("longitude", 0.0);
        } else {
            domainObj.latitude = 0.0;
            domainObj.longitude = 0.0;
        }
        
        // Extract domain info for province/city
        if (domainData.contains("domain_info")) {
            auto domainInfo = domainData["domain_info"];
            domainObj.province = domainInfo.value("province", "");
            domainObj.city = domainInfo.value("city", "");
            domainObj.domainUrl = domainInfo.value("domain_url", "");
            domainObj.pageNumber = domainInfo.value("page_number", 0);
            domainObj.rowIndex = domainInfo.value("row_index", 0);
            domainObj.rowNumber = domainInfo.value("row_number", "");
        } else {
            domainObj.province = "";
            domainObj.city = "";
            domainObj.domainUrl = "";
            domainObj.pageNumber = 0;
            domainObj.rowIndex = 0;
            domainObj.rowNumber = "";
        }
        
        // Extract grant and expiry dates
        if (domainData.contains("grant_date")) {
            auto grantDate = domainData["grant_date"];
            domainObj.grantDatePersian = grantDate.value("persian", "");
            domainObj.grantDateGregorian = grantDate.value("gregorian", "");
        } else {
            domainObj.grantDatePersian = "";
            domainObj.grantDateGregorian = "";
        }
        
        if (domainData.contains("expiry_date")) {
            auto expiryDate = domainData["expiry_date"];
            domainObj.expiryDatePersian = expiryDate.value("persian", "");
            domainObj.expiryDateGregorian = expiryDate.value("gregorian", "");
        } else {
            domainObj.expiryDatePersian = "";
            domainObj.expiryDateGregorian = "";
        }
        
        // Extract timestamp
        domainObj.extractionTimestamp = domainData.value("extraction_timestamp", "");
        
        LOG_INFO("Storage Step 8: Processing business services");
        // Store business services in metadata
        if (domainData.contains("business_services")) {
            try {
                domainObj.metadata["business_services"] = domainData["business_services"];
                LOG_INFO("Storage Step 8 completed: Business services stored in metadata");
            } catch (const std::exception& e) {
                LOG_ERROR("Error processing business_services: " + std::string(e.what()));
                domainObj.metadata = json::object(); // Reset to empty object
            }
        } else {
            LOG_INFO("Storage Step 8 completed: No business services to process");
        }
        
        LOG_INFO("Storage Step 9: Converting domain to BSON");
        auto doc = domainToBson(domainObj);
        LOG_INFO("Storage Step 9 completed: BSON conversion successful");
        
        LOG_INFO("Storage Step 10: Inserting into MongoDB");
        auto result = domainsCollection_.insert_one(doc.view());
        LOG_INFO("Storage Step 10 completed: MongoDB insertion attempt done");
        
        if (result) {
            LOG_INFO("Added domain with business info: " + domainObj.domain + " with ID: " + domainObj.id);
            return domainObj.id;
        }
        
        LOG_ERROR("Failed to insert domain with business info: " + domainObj.domain);
        return "";
    } catch (const std::exception& e) {
        LOG_ERROR("Error adding domain with business info: " + std::string(e.what()));
        return "";
    }
}

bool DomainStorage::updateDomain(const Domain& domain) {
    try {
        auto filter = document{} << "_id" << domain.id << finalize;
        auto doc = domainToBson(domain);
        
        auto result = domainsCollection_.replace_one(filter.view(), doc.view());
        
        if (result && result->modified_count() > 0) {
            LOG_DEBUG("Updated domain: " + domain.domain);
            return true;
        }
        
        LOG_WARNING("No domain updated for ID: " + domain.id);
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Error updating domain " + domain.id + ": " + std::string(e.what()));
        return false;
    }
}

bool DomainStorage::updateDomainStatus(const std::string& domainId, DomainStatus status) {
    try {
        auto filter = document{} << "_id" << domainId << finalize;
        auto update = document{} 
            << "$set" << bsoncxx::builder::stream::open_document
            << "status" << static_cast<int>(status)
            << bsoncxx::builder::stream::close_document
            << finalize;
        
        auto result = domainsCollection_.update_one(filter.view(), update.view());
        
        if (result && result->modified_count() > 0) {
            LOG_DEBUG("Updated domain status: " + domainId + " to " + std::to_string(static_cast<int>(status)));
            return true;
        }
        
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Error updating domain status " + domainId + ": " + std::string(e.what()));
        return false;
    }
}

bool DomainStorage::updateDomainCrawlInfo(const std::string& domainId, const std::string& sessionId, const std::string& jobId, int pagesCrawled) {
    try {
        auto filter = document{} << "_id" << domainId << finalize;
        auto update = document{} 
            << "$set" << bsoncxx::builder::stream::open_document
            << "crawlSessionId" << sessionId
            << "crawlJobId" << jobId
            << "pagesCrawled" << pagesCrawled
            << "lastCrawledAt" << std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()
            << bsoncxx::builder::stream::close_document
            << finalize;
        
        auto result = domainsCollection_.update_one(filter.view(), update.view());
        
        return result && result->modified_count() > 0;
    } catch (const std::exception& e) {
        LOG_ERROR("Error updating domain crawl info " + domainId + ": " + std::string(e.what()));
        return false;
    }
}

bool DomainStorage::updateDomainEmailInfo(const std::string& domainId, const std::string& emailJobId) {
    try {
        auto filter = document{} << "_id" << domainId << finalize;
        auto update = document{} 
            << "$set" << bsoncxx::builder::stream::open_document
            << "emailJobId" << emailJobId
            << "emailSentAt" << std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count()
            << bsoncxx::builder::stream::close_document
            << finalize;
        
        auto result = domainsCollection_.update_one(filter.view(), update.view());
        
        return result && result->modified_count() > 0;
    } catch (const std::exception& e) {
        LOG_ERROR("Error updating domain email info " + domainId + ": " + std::string(e.what()));
        return false;
    }
}

std::optional<Domain> DomainStorage::getDomain(const std::string& domainId) {
    try {
        auto filter = document{} << "_id" << domainId << finalize;
        auto result = domainsCollection_.find_one(filter.view());
        
        if (result) {
            return bsonToDomain(result->view());
        }
        
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting domain " + domainId + ": " + std::string(e.what()));
        return std::nullopt;
    }
}

std::optional<Domain> DomainStorage::getDomainByName(const std::string& domainName) {
    try {
        auto filter = document{} << "domain" << domainName << finalize;
        auto result = domainsCollection_.find_one(filter.view());
        
        if (result) {
            return bsonToDomain(result->view());
        }
        
        return std::nullopt;
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting domain by name " + domainName + ": " + std::string(e.what()));
        return std::nullopt;
    }
}

std::vector<Domain> DomainStorage::getDomainsByStatus(DomainStatus status, int limit, int offset) {
    std::vector<Domain> domains;
    
    try {
        auto filter = document{} << "status" << static_cast<int>(status) << finalize;
        
        mongocxx::options::find opts{};
        opts.limit(limit);
        opts.skip(offset);
        
        auto cursor = domainsCollection_.find(filter.view(), opts);
        
        for (const auto& doc : cursor) {
            domains.push_back(bsonToDomain(doc));
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting domains by status " + std::to_string(static_cast<int>(status)) + ": " + std::string(e.what()));
    }
    
    return domains;
}

std::vector<Domain> DomainStorage::getAllDomains(int limit, int offset) {
    std::vector<Domain> domains;
    
    try {
        LOG_INFO("getAllDomains called with limit: " + std::to_string(limit) + ", offset: " + std::to_string(offset));
        
        LOG_INFO("Setting up MongoDB find options");
        mongocxx::options::find opts{};
        opts.limit(limit);
        opts.skip(offset);
        LOG_INFO("MongoDB find options set");
        
        LOG_INFO("Executing MongoDB find query");
        auto cursor = domainsCollection_.find({}, opts);
        LOG_INFO("MongoDB find query executed successfully");
        
        LOG_INFO("Processing cursor results");
        int count = 0;
        for (const auto& doc : cursor) {
            try {
                LOG_DEBUG("Processing document " + std::to_string(count));
                domains.push_back(bsonToDomain(doc));
                count++;
                LOG_DEBUG("Successfully processed document " + std::to_string(count - 1));
            } catch (const std::exception& e) {
                LOG_ERROR("Error processing document " + std::to_string(count) + ": " + std::string(e.what()));
                throw;
            }
        }
        
        LOG_INFO("Successfully retrieved " + std::to_string(domains.size()) + " domains");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting all domains: " + std::string(e.what()));
        throw;
    } catch (...) {
        LOG_ERROR("Unknown error getting all domains");
        throw;
    }
    
    return domains;
}

DomainStorage::DomainStats DomainStorage::getStats() {
    DomainStats stats = {0, 0, 0, 0, 0, 0};
    
    try {
        // Count total domains
        stats.totalDomains = static_cast<int>(domainsCollection_.count_documents({}));
        
        // Count by status
        auto pendingFilter = document{} << "status" << static_cast<int>(DomainStatus::PENDING) << finalize;
        stats.pendingDomains = static_cast<int>(domainsCollection_.count_documents(pendingFilter.view()));
        
        auto crawlingFilter = document{} << "status" << static_cast<int>(DomainStatus::CRAWLING) << finalize;
        stats.crawlingDomains = static_cast<int>(domainsCollection_.count_documents(crawlingFilter.view()));
        
        auto completedFilter = document{} << "status" << static_cast<int>(DomainStatus::COMPLETED) << finalize;
        stats.completedDomains = static_cast<int>(domainsCollection_.count_documents(completedFilter.view()));
        
        auto failedFilter = document{} << "status" << static_cast<int>(DomainStatus::FAILED) << finalize;
        stats.failedDomains = static_cast<int>(domainsCollection_.count_documents(failedFilter.view()));
        
        auto emailSentFilter = document{} << "status" << static_cast<int>(DomainStatus::EMAIL_SENT) << finalize;
        stats.emailsSent = static_cast<int>(domainsCollection_.count_documents(emailSentFilter.view()));
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting domain stats: " + std::string(e.what()));
    }
    
    return stats;
}

std::vector<Domain> DomainStorage::getDomainsReadyForCrawling(int limit) {
    return getDomainsByStatus(DomainStatus::PENDING, limit, 0);
}

std::vector<Domain> DomainStorage::getDomainsReadyForEmail(int limit) {
    return getDomainsByStatus(DomainStatus::COMPLETED, limit, 0);
}

std::string DomainStorage::generateId() {
    uuid_t uuid;
    uuid_generate_random(uuid);
    char uuid_str[37];
    uuid_unparse_lower(uuid, uuid_str);
    return std::string(uuid_str);
}

bsoncxx::document::value DomainStorage::domainToBson(const Domain& domain) {
    return document{}
        << "_id" << domain.id
        << "domain" << domain.domain
        << "webmasterEmail" << domain.webmasterEmail
        << "status" << static_cast<int>(domain.status)
        << "maxPages" << domain.maxPages
        << "pagesCrawled" << domain.pagesCrawled
        << "createdAt" << std::chrono::duration_cast<std::chrono::seconds>(domain.createdAt.time_since_epoch()).count()
        << "lastCrawledAt" << std::chrono::duration_cast<std::chrono::seconds>(domain.lastCrawledAt.time_since_epoch()).count()
        << "emailSentAt" << std::chrono::duration_cast<std::chrono::seconds>(domain.emailSentAt.time_since_epoch()).count()
        << "crawlSessionId" << domain.crawlSessionId
        << "crawlJobId" << domain.crawlJobId
        << "emailJobId" << domain.emailJobId
        << "metadata" << domain.metadata.dump()
        << "errorMessage" << domain.errorMessage
        << "businessName" << domain.businessName
        << "ownerName" << domain.ownerName
        << "phone" << domain.phone
        << "businessHours" << domain.businessHours
        << "address" << domain.address
        << "province" << domain.province
        << "city" << domain.city
        << "latitude" << domain.latitude
        << "longitude" << domain.longitude
        << "grantDatePersian" << domain.grantDatePersian
        << "grantDateGregorian" << domain.grantDateGregorian
        << "expiryDatePersian" << domain.expiryDatePersian
        << "expiryDateGregorian" << domain.expiryDateGregorian
        << "extractionTimestamp" << domain.extractionTimestamp
        << "domainUrl" << domain.domainUrl
        << "pageNumber" << domain.pageNumber
        << "rowIndex" << domain.rowIndex
        << "rowNumber" << domain.rowNumber
        << finalize;
}

Domain DomainStorage::bsonToDomain(const bsoncxx::document::view& doc) {
    try {
        LOG_DEBUG("bsonToDomain started");
        Domain domain;
        
        LOG_DEBUG("Extracting basic domain fields");
        domain.id = std::string(doc["_id"].get_string().value);
        LOG_DEBUG("Got _id: " + domain.id);
        
        domain.domain = std::string(doc["domain"].get_string().value);
        LOG_DEBUG("Got domain: " + domain.domain);
        
        domain.webmasterEmail = std::string(doc["webmasterEmail"].get_string().value);
        LOG_DEBUG("Got webmasterEmail: " + domain.webmasterEmail);
        
        domain.status = static_cast<DomainStatus>(doc["status"].get_int32().value);
        LOG_DEBUG("Got status: " + std::to_string(static_cast<int>(domain.status)));
        
        domain.maxPages = doc["maxPages"].get_int32().value;
        domain.pagesCrawled = doc["pagesCrawled"].get_int32().value;
        LOG_DEBUG("Got pages info - maxPages: " + std::to_string(domain.maxPages) + ", pagesCrawled: " + std::to_string(domain.pagesCrawled));
        
        domain.createdAt = std::chrono::system_clock::from_time_t(doc["createdAt"].get_int64().value);
        domain.lastCrawledAt = std::chrono::system_clock::from_time_t(doc["lastCrawledAt"].get_int64().value);
        domain.emailSentAt = std::chrono::system_clock::from_time_t(doc["emailSentAt"].get_int64().value);
        LOG_DEBUG("Got timestamp fields");
        
        domain.crawlSessionId = std::string(doc["crawlSessionId"].get_string().value);
        domain.crawlJobId = std::string(doc["crawlJobId"].get_string().value);
        domain.emailJobId = std::string(doc["emailJobId"].get_string().value);
        LOG_DEBUG("Got ID fields");
        
        LOG_DEBUG("Processing metadata field");
        try {
            std::string metadataStr = std::string(doc["metadata"].get_string().value);
            if (!metadataStr.empty()) {
                domain.metadata = json::parse(metadataStr);
            } else {
                domain.metadata = json::object();
            }
        } catch (...) {
            LOG_DEBUG("Error parsing metadata, using empty object");
            domain.metadata = json::object();
        }
        
        domain.errorMessage = std::string(doc["errorMessage"].get_string().value);
        LOG_DEBUG("Got errorMessage");
        
        LOG_DEBUG("Processing business information fields");
        // Business Information - with safe access for optional fields
        domain.businessName = doc.find("businessName") != doc.end() ? std::string(doc["businessName"].get_string().value) : "";
        domain.ownerName = doc.find("ownerName") != doc.end() ? std::string(doc["ownerName"].get_string().value) : "";
        domain.phone = doc.find("phone") != doc.end() ? std::string(doc["phone"].get_string().value) : "";
        domain.businessHours = doc.find("businessHours") != doc.end() ? std::string(doc["businessHours"].get_string().value) : "";
        domain.address = doc.find("address") != doc.end() ? std::string(doc["address"].get_string().value) : "";
        domain.province = doc.find("province") != doc.end() ? std::string(doc["province"].get_string().value) : "";
        domain.city = doc.find("city") != doc.end() ? std::string(doc["city"].get_string().value) : "";
        LOG_DEBUG("Got business fields");
        
        LOG_DEBUG("Processing location coordinates");
        // Location coordinates
        domain.latitude = doc.find("latitude") != doc.end() ? doc["latitude"].get_double().value : 0.0;
        domain.longitude = doc.find("longitude") != doc.end() ? doc["longitude"].get_double().value : 0.0;
        LOG_DEBUG("Got coordinates");
        
        LOG_DEBUG("Processing domain registration dates");
        // Domain registration dates
        domain.grantDatePersian = doc.find("grantDatePersian") != doc.end() ? std::string(doc["grantDatePersian"].get_string().value) : "";
        domain.grantDateGregorian = doc.find("grantDateGregorian") != doc.end() ? std::string(doc["grantDateGregorian"].get_string().value) : "";
        domain.expiryDatePersian = doc.find("expiryDatePersian") != doc.end() ? std::string(doc["expiryDatePersian"].get_string().value) : "";
        domain.expiryDateGregorian = doc.find("expiryDateGregorian") != doc.end() ? std::string(doc["expiryDateGregorian"].get_string().value) : "";
        LOG_DEBUG("Got date fields");
        
        LOG_DEBUG("Processing extraction metadata");
        // Extraction metadata
        domain.extractionTimestamp = doc.find("extractionTimestamp") != doc.end() ? std::string(doc["extractionTimestamp"].get_string().value) : "";
        domain.domainUrl = doc.find("domainUrl") != doc.end() ? std::string(doc["domainUrl"].get_string().value) : "";
        domain.pageNumber = doc.find("pageNumber") != doc.end() ? doc["pageNumber"].get_int32().value : 0;
        domain.rowIndex = doc.find("rowIndex") != doc.end() ? doc["rowIndex"].get_int32().value : 0;
        domain.rowNumber = doc.find("rowNumber") != doc.end() ? std::string(doc["rowNumber"].get_string().value) : "";
        LOG_DEBUG("Got extraction metadata");
        
        LOG_DEBUG("bsonToDomain completed successfully for domain: " + domain.domain);
        return domain;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error in bsonToDomain: " + std::string(e.what()));
        throw;
    } catch (...) {
        LOG_ERROR("Unknown error in bsonToDomain");
        throw;
    }
}
