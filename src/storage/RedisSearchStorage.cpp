#include "../../include/search_engine/storage/RedisSearchStorage.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>
#include <chrono>

using namespace search_engine::storage;

namespace {
    // Helper function to escape Redis string values
    std::string escapeRedisString(const std::string& str) {
        std::string escaped = str;
        // Replace quotes and special characters
        std::replace(escaped.begin(), escaped.end(), '"', '\'');
        std::replace(escaped.begin(), escaped.end(), '\n', ' ');
        std::replace(escaped.begin(), escaped.end(), '\r', ' ');
        return escaped;
    }
    
    // Helper function to generate a hash from URL for key
    std::string urlToKey(const std::string& url) {
        std::hash<std::string> hasher;
        return std::to_string(hasher(url));
    }
    
    // Helper function to extract domain from URL
    std::string extractDomain(const std::string& url) {
        std::regex domainRegex(R"(https?://(?:www\.)?([^/]+))");
        std::smatch match;
        if (std::regex_search(url, match, domainRegex)) {
            return match[1].str();
        }
        return "";
    }
    
    // Helper function to convert time_point to Unix timestamp
    int64_t timePointToUnixTimestamp(const std::chrono::system_clock::time_point& tp) {
        return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
    }
}

RedisSearchStorage::RedisSearchStorage(
    const std::string& connectionString,
    const std::string& indexName,
    const std::string& keyPrefix
) : indexName_(indexName), keyPrefix_(keyPrefix) {
    try {
        sw::redis::ConnectionOptions opts;
        opts.host = "127.0.0.1";
        opts.port = 6379;
        opts.db = 0;
        
        // Parse connection string if provided
        if (connectionString.find("tcp://") == 0) {
            std::string hostPort = connectionString.substr(6);
            size_t colonPos = hostPort.find(':');
            if (colonPos != std::string::npos) {
                opts.host = hostPort.substr(0, colonPos);
                opts.port = std::stoi(hostPort.substr(colonPos + 1));
            }
        }
        
        redis_ = std::make_unique<sw::redis::Redis>(opts);
        
        // Test connection and initialize index if needed
        testConnection();
        if (!indexExists()) {
            initializeIndex();
        }
        
    } catch (const sw::redis::Error& e) {
        throw std::runtime_error("Failed to initialize Redis connection: " + std::string(e.what()));
    }
}

std::string RedisSearchStorage::generateDocumentKey(const std::string& url) const {
    return keyPrefix_ + urlToKey(url);
}

bool RedisSearchStorage::indexExists() const {
    try {
        // Try to get index info - if it throws, index doesn't exist
        auto reply = redis_->command("FT.INFO", indexName_);
        return true;
    } catch (const sw::redis::Error&) {
        return false;
    }
}

Result<bool> RedisSearchStorage::createSearchIndex() {
    try {
        // Create RediSearch index with schema
        std::vector<std::string> cmd = {
            "FT.CREATE", indexName_,
            "ON", "HASH",
            "PREFIX", "1", keyPrefix_,
            "SCHEMA",
            "url", "TEXT", "SORTABLE", "NOINDEX",
            "title", "TEXT", "WEIGHT", "5.0",
            "content", "TEXT", "WEIGHT", "1.0",
            "domain", "TAG", "SORTABLE",
            "keywords", "TAG",
            "description", "TEXT", "WEIGHT", "2.0",
            "language", "TAG",
            "category", "TAG",
            "indexed_at", "NUMERIC", "SORTABLE",
            "score", "NUMERIC", "SORTABLE"
        };
        
        redis_->command(cmd.begin(), cmd.end());
        return Result<bool>::Success(true, "Search index created successfully");
        
    } catch (const sw::redis::Error& e) {
        return Result<bool>::Failure("Failed to create search index: " + std::string(e.what()));
    }
}

Result<bool> RedisSearchStorage::initializeIndex() {
    return createSearchIndex();
}

Result<bool> RedisSearchStorage::indexDocument(const SearchDocument& document) {
    try {
        std::string key = generateDocumentKey(document.url);
        
        std::unordered_map<std::string, std::string> fields;
        fields["url"] = escapeRedisString(document.url);
        fields["title"] = escapeRedisString(document.title);
        fields["content"] = escapeRedisString(document.content);
        fields["domain"] = escapeRedisString(document.domain);
        fields["score"] = std::to_string(document.score);
        fields["indexed_at"] = std::to_string(timePointToUnixTimestamp(document.indexedAt));
        
        // Handle optional fields
        if (document.description) {
            fields["description"] = escapeRedisString(*document.description);
        }
        if (document.language) {
            fields["language"] = escapeRedisString(*document.language);
        }
        if (document.category) {
            fields["category"] = escapeRedisString(*document.category);
        }
        
        // Join keywords with pipes for TAG field
        if (!document.keywords.empty()) {
            std::ostringstream keywordStream;
            for (size_t i = 0; i < document.keywords.size(); ++i) {
                if (i > 0) keywordStream << "|";
                keywordStream << escapeRedisString(document.keywords[i]);
            }
            fields["keywords"] = keywordStream.str();
        }
        
        // Store the document as a Redis hash
        redis_->hmset(key, fields.begin(), fields.end());
        
        return Result<bool>::Success(true, "Document indexed successfully");
        
    } catch (const sw::redis::Error& e) {
        return Result<bool>::Failure("Failed to index document: " + std::string(e.what()));
    }
}

Result<bool> RedisSearchStorage::indexSiteProfile(const SiteProfile& profile, const std::string& content) {
    SearchDocument doc = siteProfileToSearchDocument(profile, content);
    return indexDocument(doc);
}

SearchDocument RedisSearchStorage::siteProfileToSearchDocument(
    const SiteProfile& profile, 
    const std::string& content
) {
    SearchDocument doc;
    doc.url = profile.url;
    doc.title = profile.title;
    doc.content = content;
    doc.domain = profile.domain;
    doc.keywords = profile.keywords;
    doc.description = profile.description;
    doc.language = profile.language;
    doc.category = profile.category;
    doc.indexedAt = profile.indexedAt;
    doc.score = profile.contentQuality.value_or(0.0);
    
    return doc;
}

Result<bool> RedisSearchStorage::updateDocument(const SearchDocument& document) {
    // For Redis, update is the same as index
    return indexDocument(document);
}

Result<bool> RedisSearchStorage::deleteDocument(const std::string& url) {
    try {
        std::string key = generateDocumentKey(url);
        auto deleted = redis_->del(key);
        
        if (deleted > 0) {
            return Result<bool>::Success(true, "Document deleted successfully");
        } else {
            return Result<bool>::Failure("Document not found for URL: " + url);
        }
        
    } catch (const sw::redis::Error& e) {
        return Result<bool>::Failure("Failed to delete document: " + std::string(e.what()));
    }
}

std::vector<std::string> RedisSearchStorage::buildSearchCommand(const SearchQuery& query) const {
    std::vector<std::string> cmd = {"FT.SEARCH", indexName_};
    
    // Build query string
    std::string queryStr = query.query;
    
    // Add filters
    for (const auto& filter : query.filters) {
        queryStr += " " + filter;
    }
    
    // Add language filter if specified
    if (query.language) {
        queryStr += " @language:{" + *query.language + "}";
    }
    
    // Add category filter if specified
    if (query.category) {
        queryStr += " @category:{" + *query.category + "}";
    }
    
    cmd.push_back(queryStr);
    
    // Add LIMIT
    if (query.limit > 0) {
        cmd.push_back("LIMIT");
        cmd.push_back(std::to_string(query.offset));
        cmd.push_back(std::to_string(query.limit));
    }
    
    // Add sorting (by score descending)
    cmd.push_back("SORTBY");
    cmd.push_back("score");
    cmd.push_back("DESC");
    
    // Add highlighting if requested
    if (query.highlight) {
        cmd.push_back("HIGHLIGHT");
        cmd.push_back("FIELDS");
        cmd.push_back("2");
        cmd.push_back("title");
        cmd.push_back("content");
    }
    
    return cmd;
}

SearchResult RedisSearchStorage::parseSearchResult(const std::vector<std::string>& result) const {
    SearchResult searchResult;
    
    if (result.size() < 2) {
        return searchResult;
    }
    
    // First element is the document key, skip it
    // Remaining elements are field-value pairs
    for (size_t i = 1; i < result.size(); i += 2) {
        if (i + 1 >= result.size()) break;
        
        const std::string& field = result[i];
        const std::string& value = result[i + 1];
        
        if (field == "url") {
            searchResult.url = value;
        } else if (field == "title") {
            searchResult.title = value;
        } else if (field == "content") {
            // Use first 200 characters as snippet
            searchResult.snippet = value.length() > 200 ? 
                value.substr(0, 200) + "..." : value;
        } else if (field == "domain") {
            searchResult.domain = value;
        } else if (field == "score") {
            try {
                searchResult.score = std::stod(value);
            } catch (...) {
                searchResult.score = 0.0;
            }
        }
    }
    
    return searchResult;
}

Result<SearchResponse> RedisSearchStorage::search(const SearchQuery& query) {
    try {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        auto cmd = buildSearchCommand(query);
        auto reply = redis_->command(cmd.begin(), cmd.end());
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        SearchResponse response;
        response.queryTime = duration.count();
        response.indexName = indexName_;
        
        // Parse reply - access raw redis reply
        if (reply && reply->type == REDIS_REPLY_ARRAY) {
            auto replyArray = reply.get();
            if (replyArray->elements > 0) {
                // First element is total count
                if (replyArray->element[0]->type == REDIS_REPLY_INTEGER) {
                    response.totalResults = replyArray->element[0]->integer;
                }
                
                // Remaining elements are documents (groups of key + fields)
                for (size_t i = 1; i < replyArray->elements; i += 2) {
                    if (i + 1 >= replyArray->elements) break;
                    
                    std::vector<std::string> docData;
                    
                    // Document key
                    if (replyArray->element[i]->type == REDIS_REPLY_STRING) {
                        docData.push_back(std::string(replyArray->element[i]->str, replyArray->element[i]->len));
                    }
                    
                    // Document fields
                    if (replyArray->element[i + 1]->type == REDIS_REPLY_ARRAY) {
                        auto fieldsReply = replyArray->element[i + 1];
                        for (size_t j = 0; j < fieldsReply->elements; ++j) {
                            if (fieldsReply->element[j]->type == REDIS_REPLY_STRING) {
                                docData.push_back(std::string(fieldsReply->element[j]->str, fieldsReply->element[j]->len));
                            }
                        }
                    }
                    
                    auto searchResult = parseSearchResult(docData);
                    if (!searchResult.url.empty()) {
                        response.results.push_back(searchResult);
                    }
                }
            }
        }
        
        return Result<SearchResponse>::Success(
            std::move(response),
            "Search completed successfully"
        );
        
    } catch (const sw::redis::Error& e) {
        return Result<SearchResponse>::Failure("Search failed: " + std::string(e.what()));
    }
}

Result<SearchResponse> RedisSearchStorage::searchSimple(const std::string& query, int limit) {
    SearchQuery searchQuery;
    searchQuery.query = query;
    searchQuery.limit = limit;
    searchQuery.highlight = true;
    
    return search(searchQuery);
}

Result<std::vector<std::string>> RedisSearchStorage::suggest(const std::string& prefix, int limit) {
    try {
        // Use FT.SUGGET for autocomplete suggestions
        std::vector<std::string> cmd = {
            "FT.SUGGET", indexName_ + ":suggestions", prefix, "MAX", std::to_string(limit)
        };
        
        auto reply = redis_->command(cmd.begin(), cmd.end());
        
        std::vector<std::string> suggestions;
        if (reply && reply->type == REDIS_REPLY_ARRAY) {
            auto replyArray = reply.get();
            for (size_t i = 0; i < replyArray->elements; ++i) {
                if (replyArray->element[i]->type == REDIS_REPLY_STRING) {
                    suggestions.push_back(std::string(replyArray->element[i]->str, replyArray->element[i]->len));
                }
            }
        }
        
        return Result<std::vector<std::string>>::Success(
            std::move(suggestions),
            "Suggestions retrieved successfully"
        );
        
    } catch (const sw::redis::Error& e) {
        return Result<std::vector<std::string>>::Failure("Suggestion failed: " + std::string(e.what()));
    }
}

Result<bool> RedisSearchStorage::dropIndex() {
    try {
        redis_->command("FT.DROPINDEX", indexName_);
        return Result<bool>::Success(true, "Index dropped successfully");
    } catch (const sw::redis::Error& e) {
        return Result<bool>::Failure("Failed to drop index: " + std::string(e.what()));
    }
}

Result<int64_t> RedisSearchStorage::getDocumentCount() {
    try {
        auto reply = redis_->command("FT.INFO", indexName_);
        
        // Parse the INFO response to find document count
        if (reply && reply->type == REDIS_REPLY_ARRAY) {
            auto replyArray = reply.get();
            for (size_t i = 0; i < replyArray->elements - 1; ++i) {
                if (replyArray->element[i]->type == REDIS_REPLY_STRING) {
                    std::string key(replyArray->element[i]->str, replyArray->element[i]->len);
                    if (key == "num_docs" && replyArray->element[i + 1]->type == REDIS_REPLY_INTEGER) {
                        return Result<int64_t>::Success(
                            replyArray->element[i + 1]->integer,
                            "Document count retrieved successfully"
                        );
                    }
                }
            }
        }
        
        return Result<int64_t>::Failure("Could not parse document count from index info");
        
    } catch (const sw::redis::Error& e) {
        return Result<int64_t>::Failure("Failed to get document count: " + std::string(e.what()));
    }
}

Result<std::unordered_map<std::string, std::string>> RedisSearchStorage::getIndexInfo() {
    try {
        auto reply = redis_->command("FT.INFO", indexName_);
        
        std::unordered_map<std::string, std::string> info;
        
        if (reply && reply->type == REDIS_REPLY_ARRAY) {
            auto replyArray = reply.get();
            for (size_t i = 0; i < replyArray->elements - 1; i += 2) {
                if (replyArray->element[i]->type == REDIS_REPLY_STRING && 
                    replyArray->element[i + 1]->type == REDIS_REPLY_STRING) {
                    std::string key(replyArray->element[i]->str, replyArray->element[i]->len);
                    std::string value(replyArray->element[i + 1]->str, replyArray->element[i + 1]->len);
                    info[key] = value;
                }
            }
        }
        
        return Result<std::unordered_map<std::string, std::string>>::Success(
            std::move(info),
            "Index info retrieved successfully"
        );
        
    } catch (const sw::redis::Error& e) {
        return Result<std::unordered_map<std::string, std::string>>::Failure(
            "Failed to get index info: " + std::string(e.what())
        );
    }
}

Result<bool> RedisSearchStorage::testConnection() {
    try {
        auto pong = redis_->ping();
        return Result<bool>::Success(true, "Redis connection is healthy");
    } catch (const sw::redis::Error& e) {
        return Result<bool>::Failure("Redis connection test failed: " + std::string(e.what()));
    }
}

Result<bool> RedisSearchStorage::ping() {
    return testConnection();
}

Result<std::vector<bool>> RedisSearchStorage::indexDocuments(const std::vector<SearchDocument>& documents) {
    std::vector<bool> results;
    
    try {
        for (const auto& document : documents) {
            auto result = indexDocument(document);
            results.push_back(result.success);
        }
        
        return Result<std::vector<bool>>::Success(
            std::move(results),
            "Batch document indexing completed"
        );
        
    } catch (const sw::redis::Error& e) {
        return Result<std::vector<bool>>::Failure("Batch indexing failed: " + std::string(e.what()));
    }
}

Result<bool> RedisSearchStorage::deleteDocumentsByDomain(const std::string& domain) {
    try {
        // Use FT.SEARCH to find all documents for this domain
        std::vector<std::string> searchCmd = {
            "FT.SEARCH", indexName_, "@domain:" + escapeRedisString(domain), "RETURN", "1", "url"
        };
        
        auto reply = redis_->command(searchCmd.begin(), searchCmd.end());
        
        std::vector<std::string> urlsToDelete;
        
        if (reply && reply->type == REDIS_REPLY_ARRAY) {
            auto replyArray = reply.get();
            if (replyArray->elements > 0) {
                // Skip the count (first element) and iterate through document results
                for (size_t i = 1; i < replyArray->elements; i += 2) {
                    if (i + 1 >= replyArray->elements) break;
                    
                    // Check if the next element contains fields
                    if (replyArray->element[i + 1]->type == REDIS_REPLY_ARRAY) {
                        auto fieldsReply = replyArray->element[i + 1];
                        for (size_t j = 0; j < fieldsReply->elements - 1; j += 2) {
                            if (fieldsReply->element[j]->type == REDIS_REPLY_STRING &&
                                fieldsReply->element[j + 1]->type == REDIS_REPLY_STRING) {
                                std::string field(fieldsReply->element[j]->str, fieldsReply->element[j]->len);
                                std::string value(fieldsReply->element[j + 1]->str, fieldsReply->element[j + 1]->len);
                                
                                if (field == "url") {
                                    urlsToDelete.push_back(value);
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Delete each document found
        int deletedCount = 0;
        for (const auto& url : urlsToDelete) {
            auto deleteResult = deleteDocument(url);
            if (deleteResult.success) {
                deletedCount++;
            }
        }
        
        return Result<bool>::Success(
            true,
            "Deleted " + std::to_string(deletedCount) + " documents for domain: " + domain
        );
        
    } catch (const sw::redis::Error& e) {
        return Result<bool>::Failure("Failed to delete documents by domain: " + std::string(e.what()));
    }
}

Result<bool> RedisSearchStorage::reindexAll() {
    try {
        // Get all document keys
        std::vector<std::string> searchCmd = {
            "FT.SEARCH", indexName_, "*", "RETURN", "0"
        };
        
        auto reply = redis_->command(searchCmd.begin(), searchCmd.end());
        
        if (reply && reply->type == REDIS_REPLY_ARRAY) {
            auto replyArray = reply.get();
            if (replyArray->elements > 0 && replyArray->element[0]->type == REDIS_REPLY_INTEGER) {
                int64_t totalDocs = replyArray->element[0]->integer;
                
                if (totalDocs > 0) {
                    // Drop and recreate the index
                    auto dropResult = dropIndex();
                    if (!dropResult.success) {
                        return Result<bool>::Failure("Failed to drop index for reindexing: " + dropResult.message);
                    }
                    
                    auto initResult = initializeIndex();
                    if (!initResult.success) {
                        return Result<bool>::Failure("Failed to recreate index for reindexing: " + initResult.message);
                    }
                    
                    return Result<bool>::Success(
                        true,
                        "Reindexing completed. Index recreated for " + std::to_string(totalDocs) + " documents."
                    );
                }
            }
        }
        
        return Result<bool>::Success(true, "No documents found to reindex");
        
    } catch (const sw::redis::Error& e) {
        return Result<bool>::Failure("Reindexing failed: " + std::string(e.what()));
    }
} 