#include "../../include/search_engine/storage/ContentStorage.h"
#include <regex>
#include <algorithm>
#include <sstream>

using namespace search_engine::storage;

namespace {
    // Helper function to extract domain from URL
    std::string extractDomain(const std::string& url) {
        std::regex domainRegex(R"(https?://(?:www\.)?([^/]+))");
        std::smatch match;
        if (std::regex_search(url, match, domainRegex)) {
            return match[1].str();
        }
        return "";
    }
    
    // Helper function to check if URL uses HTTPS
    bool hasSSL(const std::string& url) {
        return url.find("https://") == 0;
    }
    
    // Helper function to count words in text
    int countWords(const std::string& text) {
        std::istringstream stream(text);
        std::string word;
        int count = 0;
        while (stream >> word) {
            ++count;
        }
        return count;
    }
    
    // Helper function to extract keywords from text (simple implementation)
    std::vector<std::string> extractKeywords(const std::string& text, int maxKeywords = 10) {
        std::vector<std::string> keywords;
        std::istringstream stream(text);
        std::string word;
        std::unordered_map<std::string, int> wordCount;
        
        // Count word frequencies
        while (stream >> word) {
            // Remove punctuation and convert to lowercase
            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            
            // Skip short words and common stop words
            if (word.length() > 3 && 
                word != "the" && word != "and" && word != "for" && 
                word != "are" && word != "but" && word != "not" && 
                word != "you" && word != "all" && word != "can" &&
                word != "had" && word != "her" && word != "was" &&
                word != "one" && word != "our" && word != "out" &&
                word != "day" && word != "get" && word != "has" &&
                word != "him" && word != "his" && word != "how" &&
                word != "its" && word != "may" && word != "new" &&
                word != "now" && word != "old" && word != "see" &&
                word != "two" && word != "who" && word != "boy" &&
                word != "did" && word != "she" && word != "use" &&
                word != "her" && word != "how" && word != "man" &&
                word != "way") {
                wordCount[word]++;
            }
        }
        
        // Sort by frequency and take top keywords
        std::vector<std::pair<std::string, int>> sortedWords(wordCount.begin(), wordCount.end());
        std::sort(sortedWords.begin(), sortedWords.end(), 
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        for (int i = 0; i < std::min(maxKeywords, static_cast<int>(sortedWords.size())); ++i) {
            keywords.push_back(sortedWords[i].first);
        }
        
        return keywords;
    }
}

ContentStorage::ContentStorage(
    const std::string& mongoConnectionString,
    const std::string& mongoDatabaseName
#ifdef REDIS_AVAILABLE
    ,const std::string& redisConnectionString,
    const std::string& redisIndexName
#endif
) {
    try {
        mongoStorage_ = std::make_unique<MongoDBStorage>(mongoConnectionString, mongoDatabaseName);
#ifdef REDIS_AVAILABLE
        redisStorage_ = std::make_unique<RedisSearchStorage>(redisConnectionString, redisIndexName);
#endif
        
        // Test connections
        auto mongoTest = mongoStorage_->testConnection();
        if (!mongoTest.success) {
            throw std::runtime_error("MongoDB connection failed: " + mongoTest.message);
        }
        
#ifdef REDIS_AVAILABLE
        auto redisTest = redisStorage_->testConnection();
        if (!redisTest.success) {
            throw std::runtime_error("Redis connection failed: " + redisTest.message);
        }
#endif
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to initialize ContentStorage: " + std::string(e.what()));
    }
}

SiteProfile ContentStorage::crawlResultToSiteProfile(const CrawlResult& crawlResult) const {
    SiteProfile profile;
    
    // Basic information
    profile.url = crawlResult.url;
    profile.domain = extractDomain(crawlResult.url);
    profile.title = crawlResult.title.value_or("");
    profile.description = crawlResult.metaDescription;
    
    // Technical metadata
    profile.crawlMetadata.lastCrawlTime = crawlResult.crawlTime;
    profile.crawlMetadata.firstCrawlTime = crawlResult.crawlTime; // Will be updated if exists
    profile.crawlMetadata.lastCrawlStatus = crawlResult.success ? CrawlStatus::SUCCESS : CrawlStatus::FAILED;
    profile.crawlMetadata.lastErrorMessage = crawlResult.errorMessage;
    profile.crawlMetadata.crawlCount = 1; // Will be updated if exists
    profile.crawlMetadata.crawlIntervalHours = 24.0; // Default interval
    profile.crawlMetadata.userAgent = "SearchEngine-Bot/1.0";
    profile.crawlMetadata.httpStatusCode = crawlResult.statusCode;
    profile.crawlMetadata.contentSize = crawlResult.contentSize;
    profile.crawlMetadata.contentType = crawlResult.contentType;
    profile.crawlMetadata.crawlDurationMs = 0.0; // Not available in CrawlResult
    
    // Extract keywords from content
    if (crawlResult.textContent) {
        profile.keywords = extractKeywords(*crawlResult.textContent);
        profile.wordCount = countWords(*crawlResult.textContent);
    }
    
    // Set technical flags
    profile.hasSSL = hasSSL(crawlResult.url);
    profile.isIndexed = crawlResult.success;
    profile.lastModified = crawlResult.crawlTime;
    profile.indexedAt = crawlResult.crawlTime;
    
    // Extract outbound links
    profile.outboundLinks = crawlResult.links;
    
    // Set default quality score based on content length and status
    if (crawlResult.success && crawlResult.textContent && !crawlResult.textContent->empty()) {
        double contentLength = static_cast<double>(crawlResult.textContent->length());
        profile.contentQuality = std::min(1.0, contentLength / 10000.0); // Normalize to 0-1
    } else {
        profile.contentQuality = 0.0;
    }
    
    return profile;
}

std::string ContentStorage::extractSearchableContent(const CrawlResult& crawlResult) const {
    std::ostringstream content;
    
    // Add title with higher weight
    if (crawlResult.title) {
        content << *crawlResult.title << " ";
        content << *crawlResult.title << " "; // Add twice for weight
    }
    
    // Add meta description
    if (crawlResult.metaDescription) {
        content << *crawlResult.metaDescription << " ";
    }
    
    // Add main text content
    if (crawlResult.textContent) {
        content << *crawlResult.textContent;
    }
    
    return content.str();
}

Result<std::string> ContentStorage::storeCrawlResult(const CrawlResult& crawlResult) {
    try {
        // Convert CrawlResult to SiteProfile
        SiteProfile profile = crawlResultToSiteProfile(crawlResult);
        
        // Check if site profile already exists
        auto existingProfile = mongoStorage_->getSiteProfile(crawlResult.url);
        if (existingProfile.success) {
            // Update existing profile
            auto existing = existingProfile.value;
            
            // Update crawl metadata
            profile.id = existing.id;
            profile.crawlMetadata.firstCrawlTime = existing.crawlMetadata.firstCrawlTime;
            profile.crawlMetadata.crawlCount = existing.crawlMetadata.crawlCount + 1;
            
            // Keep existing fields that might have been manually set
            if (!existing.category.has_value() && profile.category.has_value()) {
                profile.category = existing.category;
            }
            if (existing.pageRank.has_value()) {
                profile.pageRank = existing.pageRank;
            }
            if (existing.inboundLinkCount.has_value()) {
                profile.inboundLinkCount = existing.inboundLinkCount;
            }
            
            // Update the profile in MongoDB
            auto mongoResult = mongoStorage_->updateSiteProfile(profile);
            if (!mongoResult.success) {
                return Result<std::string>::Failure("Failed to update in MongoDB: " + mongoResult.message);
            }
        } else {
            // Store new profile in MongoDB
            auto mongoResult = mongoStorage_->storeSiteProfile(profile);
            if (!mongoResult.success) {
                return Result<std::string>::Failure("Failed to store in MongoDB: " + mongoResult.message);
            }
            profile.id = mongoResult.value;
        }
        
        // Index in Redis if successful and has content
#ifdef REDIS_AVAILABLE
        if (crawlResult.success && crawlResult.textContent) {
            std::string searchableContent = extractSearchableContent(crawlResult);
            auto redisResult = redisStorage_->indexSiteProfile(profile, searchableContent);
            if (!redisResult.success) {
                // Log warning but don't fail the operation
                // In a real system, you might want to queue this for retry
            }
        }
#endif
        
        return Result<std::string>::Success(
            profile.id.value_or(""),
            "Crawl result stored successfully"
        );
        
    } catch (const std::exception& e) {
        return Result<std::string>::Failure("Exception in storeCrawlResult: " + std::string(e.what()));
    }
}

Result<bool> ContentStorage::updateCrawlResult(const CrawlResult& crawlResult) {
    // For updates, we use the same logic as store
    auto result = storeCrawlResult(crawlResult);
    return Result<bool>::Success(result.success, result.message);
}

Result<SiteProfile> ContentStorage::getSiteProfile(const std::string& url) {
    return mongoStorage_->getSiteProfile(url);
}

Result<std::vector<SiteProfile>> ContentStorage::getSiteProfilesByDomain(const std::string& domain) {
    return mongoStorage_->getSiteProfilesByDomain(domain);
}

Result<std::vector<SiteProfile>> ContentStorage::getSiteProfilesByCrawlStatus(CrawlStatus status) {
    return mongoStorage_->getSiteProfilesByCrawlStatus(status);
}

Result<int64_t> ContentStorage::getTotalSiteCount() {
    return mongoStorage_->getTotalSiteCount();
}

#ifdef REDIS_AVAILABLE
Result<SearchResponse> ContentStorage::search(const SearchQuery& query) {
    return redisStorage_->search(query);
}

Result<SearchResponse> ContentStorage::searchSimple(const std::string& query, int limit) {
    return redisStorage_->searchSimple(query, limit);
}

Result<std::vector<std::string>> ContentStorage::suggest(const std::string& prefix, int limit) {
    return redisStorage_->suggest(prefix, limit);
}
#endif

Result<std::vector<std::string>> ContentStorage::storeCrawlResults(const std::vector<CrawlResult>& crawlResults) {
    std::vector<std::string> ids;
    
    for (const auto& crawlResult : crawlResults) {
        auto result = storeCrawlResult(crawlResult);
        if (result.success) {
            ids.push_back(result.value);
        } else {
            return Result<std::vector<std::string>>::Failure(
                "Failed to store crawl result for " + crawlResult.url + ": " + result.message
            );
        }
    }
    
    return Result<std::vector<std::string>>::Success(
        std::move(ids),
        "All crawl results stored successfully"
    );
}

Result<bool> ContentStorage::initializeIndexes() {
    try {
        // Initialize MongoDB indexes
        auto mongoResult = mongoStorage_->ensureIndexes();
        if (!mongoResult.success) {
            return Result<bool>::Failure("Failed to initialize MongoDB indexes: " + mongoResult.message);
        }
        
#ifdef REDIS_AVAILABLE
        // Initialize Redis search index
        auto redisResult = redisStorage_->initializeIndex();
        if (!redisResult.success) {
            return Result<bool>::Failure("Failed to initialize Redis index: " + redisResult.message);
        }
#endif
        
        return Result<bool>::Success(true, "All indexes initialized successfully");
        
    } catch (const std::exception& e) {
        return Result<bool>::Failure("Exception in initializeIndexes: " + std::string(e.what()));
    }
}

#ifdef REDIS_AVAILABLE
Result<bool> ContentStorage::reindexAll() {
    return redisStorage_->reindexAll();
}

Result<bool> ContentStorage::dropIndexes() {
    return redisStorage_->dropIndexes();
}
#endif

Result<bool> ContentStorage::testConnections() {
    try {
        // Test MongoDB connection
        auto mongoResult = mongoStorage_->testConnection();
        if (!mongoResult.success) {
            return Result<bool>::Failure("MongoDB connection failed: " + mongoResult.message);
        }
        
#ifdef REDIS_AVAILABLE
        // Test Redis connection
        auto redisResult = redisStorage_->testConnection();
        if (!redisResult.success) {
            return Result<bool>::Failure("Redis connection failed: " + redisResult.message);
        }
#endif
        
        return Result<bool>::Success(true, "All connections are healthy");
        
    } catch (const std::exception& e) {
        return Result<bool>::Failure("Exception in testConnections: " + std::string(e.what()));
    }
}

Result<std::unordered_map<std::string, std::string>> ContentStorage::getStorageStats() {
    try {
        std::unordered_map<std::string, std::string> stats;
        
        // Get MongoDB stats
        auto mongoCount = mongoStorage_->getTotalSiteCount();
        if (mongoCount.success) {
            stats["mongodb_total_documents"] = std::to_string(mongoCount.value);
        }
        
        auto mongoSuccessCount = mongoStorage_->getSiteCountByStatus(CrawlStatus::SUCCESS);
        if (mongoSuccessCount.success) {
            stats["mongodb_successful_crawls"] = std::to_string(mongoSuccessCount.value);
        }
        
#ifdef REDIS_AVAILABLE
        // Get Redis stats
        auto redisCount = redisStorage_->getDocumentCount();
        if (redisCount.success) {
            stats["redis_indexed_documents"] = std::to_string(redisCount.value);
        }
        
        auto redisInfo = redisStorage_->getIndexInfo();
        if (redisInfo.success) {
            auto info = redisInfo.value;
            for (const auto& [key, value] : info) {
                stats["redis_" + key] = value;
            }
        }
#endif
        
        return Result<std::unordered_map<std::string, std::string>>::Success(
            std::move(stats),
            "Storage statistics retrieved successfully"
        );
        
    } catch (const std::exception& e) {
        return Result<std::unordered_map<std::string, std::string>>::Failure(
            "Exception in getStorageStats: " + std::string(e.what())
        );
    }
}

Result<bool> ContentStorage::deleteSiteData(const std::string& url) {
    try {
        // Delete from MongoDB
        auto mongoResult = mongoStorage_->deleteSiteProfile(url);
        
#ifdef REDIS_AVAILABLE
        // Delete from Redis (ignore if not found)
        auto redisResult = redisStorage_->deleteDocument(url);
#endif
        
        if (mongoResult.success) {
            return Result<bool>::Success(true, "Site data deleted successfully");
        } else {
            return Result<bool>::Failure("Failed to delete from MongoDB: " + mongoResult.message);
        }
        
    } catch (const std::exception& e) {
        return Result<bool>::Failure("Exception in deleteSiteData: " + std::string(e.what()));
    }
}

Result<bool> ContentStorage::deleteDomainData(const std::string& domain) {
    try {
        // Get all profiles for the domain first
        auto profiles = mongoStorage_->getSiteProfilesByDomain(domain);
        if (!profiles.success) {
            return Result<bool>::Failure("Failed to get profiles for domain: " + profiles.message);
        }
        
        // Delete each profile
        for (const auto& profile : profiles.value) {
            auto deleteResult = deleteSiteData(profile.url);
            if (!deleteResult.success) {
                return Result<bool>::Failure("Failed to delete site data for " + profile.url);
            }
        }
        
#ifdef REDIS_AVAILABLE
        // Delete from Redis by domain
        auto redisResult = redisStorage_->deleteDocumentsByDomain(domain);
#endif
        
        return Result<bool>::Success(true, "Domain data deleted successfully");
        
    } catch (const std::exception& e) {
        return Result<bool>::Failure("Exception in deleteDomainData: " + std::string(e.what()));
    }
} 