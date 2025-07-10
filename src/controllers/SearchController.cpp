#include "SearchController.h"
#include "../../include/Logger.h"
#include "../../src/crawler/Crawler.h"
#include "../../src/crawler/models/CrawlConfig.h"
#include <cstdlib>
#include <chrono>
#include <iomanip>

using namespace hatef::search;

// Static SearchClient instance
static std::unique_ptr<SearchClient> g_searchClient;
static std::once_flag g_initFlag;

// Static Crawler instance
static std::unique_ptr<Crawler> g_crawler;
static std::once_flag g_crawlerInitFlag;

SearchController::SearchController() {
    // Initialize SearchClient once
    std::call_once(g_initFlag, []() {
        RedisConfig config;
        
        const char* redisUri = std::getenv("SEARCH_REDIS_URI");
        if (redisUri) {
            config.uri = redisUri;
            LOG_INFO("Using Redis URI from environment: " + config.uri);
        } else {
            LOG_INFO("Using default Redis URI: " + config.uri);
        }
        
        const char* poolSize = std::getenv("SEARCH_REDIS_POOL_SIZE");
        if (poolSize) {
            try {
                config.pool_size = std::stoul(poolSize);
                LOG_INFO("Using Redis pool size from environment: " + std::to_string(config.pool_size));
            } catch (...) {
                LOG_WARNING("Invalid SEARCH_REDIS_POOL_SIZE, using default: " + std::to_string(config.pool_size));
            }
        }
        
        try {
            g_searchClient = std::make_unique<SearchClient>(config);
            LOG_INFO("SearchClient initialized successfully");
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to initialize SearchClient: " + std::string(e.what()));
            throw;
        }
    });
    
    // Initialize Crawler once
    std::call_once(g_crawlerInitFlag, []() {
        CrawlConfig config;
        config.maxPages = 1000;  // Default max pages
        config.maxDepth = 3;     // Default max depth
        config.userAgent = "SearchEngineCrawler/1.0";
        
        try {
            g_crawler = std::make_unique<Crawler>(config);
            LOG_INFO("Crawler initialized successfully");
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to initialize Crawler: " + std::string(e.what()));
            throw;
        }
    });
}

void SearchController::addSiteToCrawl(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    LOG_INFO("SearchController::addSiteToCrawl called");
    
    // Read the request body
    std::string buffer;
    res->onData([this, res, buffer = std::move(buffer)](std::string_view data, bool last) mutable {
        buffer.append(data.data(), data.length());
        
        if (last) {
            try {
                // Parse JSON body
                auto jsonBody = nlohmann::json::parse(buffer);
                
                // Validate required fields
                if (!jsonBody.contains("url") || jsonBody["url"].empty()) {
                    badRequest(res, "URL is required");
                    return;
                }
                
                std::string url = jsonBody["url"];
                
                // Optional parameters
                int maxPages = jsonBody.value("maxPages", 1000);
                int maxDepth = jsonBody.value("maxDepth", 3);
                
                // Validate parameters
                if (maxPages < 1 || maxPages > 10000) {
                    badRequest(res, "maxPages must be between 1 and 10000");
                    return;
                }
                
                if (maxDepth < 1 || maxDepth > 10) {
                    badRequest(res, "maxDepth must be between 1 and 10");
                    return;
                }
                
                // Add URL to crawler
                if (g_crawler) {
                    g_crawler->addSeedURL(url);
                    
                    // Start crawling if not already running
                    g_crawler->start();
                    
                    LOG_INFO("Added site to crawl: " + url + " (maxPages: " + std::to_string(maxPages) + 
                             ", maxDepth: " + std::to_string(maxDepth) + ")");
                    
                    // Return success response
                    nlohmann::json response = {
                        {"success", true},
                        {"message", "Site added to crawl queue successfully"},
                        {"data", {
                            {"url", url},
                            {"maxPages", maxPages},
                            {"maxDepth", maxDepth},
                            {"status", "queued"}
                        }}
                    };
                    
                    json(res, response);
                } else {
                    serverError(res, "Crawler not initialized");
                }
                
            } catch (const nlohmann::json::parse_error& e) {
                LOG_ERROR("Failed to parse JSON: " + std::string(e.what()));
                badRequest(res, "Invalid JSON format");
            } catch (const std::exception& e) {
                LOG_ERROR("Unexpected error in addSiteToCrawl: " + std::string(e.what()));
                serverError(res, "An unexpected error occurred");
            }
        }
    });
    
    res->onAborted([]() {
        LOG_WARNING("Add site to crawl request aborted");
    });
}

void SearchController::search(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    LOG_INFO("SearchController::search called");
    
    // Parse query parameters
    auto params = parseQuery(req);
    
    // Check for required 'q' parameter
    auto qIt = params.find("q");
    if (qIt == params.end() || qIt->second.empty()) {
        nlohmann::json error = {
            {"error", {
                {"code", "INVALID_REQUEST"},
                {"message", "Invalid request parameters"},
                {"details", {
                    {"q", "Query parameter is required"}
                }}
            }}
        };
        
        json(res, error, "400 Bad Request");
        LOG_WARNING("Search request rejected: missing 'q' parameter");
        return;
    }
    
    // Parse pagination
    int page = 1;
    int limit = 10;
    
    auto pageIt = params.find("page");
    if (pageIt != params.end()) {
        try {
            page = std::stoi(pageIt->second);
            if (page < 1 || page > 1000) {
                badRequest(res, "Page must be between 1 and 1000");
                return;
            }
        } catch (...) {
            badRequest(res, "Invalid page parameter");
            return;
        }
    }
    
    auto limitIt = params.find("limit");
    if (limitIt != params.end()) {
        try {
            limit = std::stoi(limitIt->second);
            if (limit < 1 || limit > 100) {
                badRequest(res, "Limit must be between 1 and 100");
                return;
            }
        } catch (...) {
            badRequest(res, "Invalid limit parameter");
            return;
        }
    }
    
    try {
        // Get index name from environment or use default
        const char* indexName = std::getenv("SEARCH_INDEX_NAME");
        std::string searchIndex = indexName ? indexName : "search_index";
        
        // Build search arguments
        std::vector<std::string> searchArgs;
        
        // Add LIMIT for pagination
        int offset = (page - 1) * limit;
        searchArgs.push_back("LIMIT");
        searchArgs.push_back(std::to_string(offset));
        searchArgs.push_back(std::to_string(limit));
        
        // Add RETURN to specify which fields to return
        searchArgs.push_back("RETURN");
        searchArgs.push_back("4");
        searchArgs.push_back("url");
        searchArgs.push_back("title");
        searchArgs.push_back("content");
        searchArgs.push_back("score");
        
        // Execute search
        std::string rawResult = g_searchClient->search(searchIndex, qIt->second, searchArgs);
        
        // Parse and format response
        nlohmann::json response = parseRedisSearchResponse(rawResult, page, limit);
        
        LOG_INFO("Search request successful: q=" + qIt->second + 
                 ", page=" + std::to_string(page) + 
                 ", limit=" + std::to_string(limit));
        
        json(res, response);
        
    } catch (const SearchError& e) {
        LOG_ERROR("Search error: " + std::string(e.what()));
        
        std::string errorMsg = e.what();
        if (errorMsg.find("no such index") != std::string::npos || 
            errorMsg.find("Unknown Index") != std::string::npos) {
            // Return empty results for non-existent index
            nlohmann::json response;
            response["meta"]["total"] = 0;
            response["meta"]["page"] = page;
            response["meta"]["pageSize"] = limit;
            response["results"] = nlohmann::json::array();
            
            json(res, response);
        } else {
            serverError(res, "Search operation failed");
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Unexpected error: " + std::string(e.what()));
        serverError(res, "An unexpected error occurred");
    }
}

void SearchController::getCrawlStatus(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    LOG_INFO("SearchController::getCrawlStatus called");
    
    try {
        // Parse query parameters
        auto params = parseQuery(req);
        
        // Get optional parameters
        bool includeLogs = params.find("logs") != params.end() && params["logs"] == "true";
        bool includeResults = params.find("results") != params.end() && params["results"] == "true";
        int maxResults = 50; // Default limit
        
        auto maxResultsIt = params.find("maxResults");
        if (maxResultsIt != params.end()) {
            try {
                maxResults = std::stoi(maxResultsIt->second);
                if (maxResults < 1 || maxResults > 1000) {
                    maxResults = 50; // Reset to default if invalid
                }
            } catch (...) {
                maxResults = 50; // Reset to default if parsing fails
            }
        }
        
        nlohmann::json response;
        
        if (g_crawler) {
            // Get crawl results
            auto results = g_crawler->getResults();
            
            // Basic status information
            response["status"] = {
                {"isRunning", true}, // We'll need to add a method to check this
                {"totalCrawled", static_cast<int>(results.size())},
                {"lastUpdate", std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count()}
            };
            
            // Include crawl results if requested
            if (includeResults) {
                nlohmann::json resultsArray = nlohmann::json::array();
                
                // Get the most recent results (up to maxResults)
                int startIndex = std::max(0, static_cast<int>(results.size()) - maxResults);
                for (size_t i = startIndex; i < results.size(); ++i) {
                    const auto& result = results[i];
                    
                    nlohmann::json resultJson = {
                        {"url", result.url},
                        {"statusCode", result.statusCode},
                        {"status", result.success ? "success" : "failed"},
                        {"crawlTime", std::chrono::duration_cast<std::chrono::seconds>(
                            result.crawlTime.time_since_epoch()).count()},
                        {"contentSize", static_cast<int>(result.contentSize)},
                        {"linksFound", static_cast<int>(result.links.size())}
                    };
                    
                    // Add optional fields if they exist
                    if (result.title.has_value()) {
                        resultJson["title"] = result.title.value();
                    }
                    
                    if (result.rawContent.has_value()) {
                        resultJson["contentLength"] = static_cast<int>(result.rawContent.value().length());
                    }
                    
                    if (!result.success && result.errorMessage.has_value()) {
                        resultJson["error"] = result.errorMessage.value();
                    }
                    
                    resultsArray.push_back(resultJson);
                }
                
                response["results"] = resultsArray;
            }
            
            // Include recent logs if requested
            if (includeLogs) {
                // For now, we'll return a placeholder for logs
                // In a real implementation, you'd want to capture logs from the crawler
                response["logs"] = {
                    {"message", "Log collection not yet implemented"},
                    {"note", "Consider implementing a log collector in the Crawler class"}
                };
            }
            
            // Crawl statistics
            int successfulCrawls = 0;
            int failedCrawls = 0;
            int totalLinks = 0;
            
            for (const auto& result : results) {
                if (result.success) {
                    successfulCrawls++;
                    totalLinks += result.links.size();
                } else {
                    failedCrawls++;
                }
            }
            
            response["statistics"] = {
                {"successfulCrawls", successfulCrawls},
                {"failedCrawls", failedCrawls},
                {"totalLinksFound", totalLinks},
                {"successRate", results.empty() ? 0.0 : (double)successfulCrawls / results.size() * 100.0}
            };
            
        } else {
            response["status"] = {
                {"isRunning", false},
                {"message", "Crawler not initialized"},
                {"totalCrawled", 0}
            };
        }
        
        json(res, response);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error in getCrawlStatus: " + std::string(e.what()));
        serverError(res, "Failed to get crawl status");
    }
}

nlohmann::json SearchController::parseRedisSearchResponse(const std::string& rawResponse, int page, int limit) {
    nlohmann::json response;
    response["meta"] = nlohmann::json::object();
    response["results"] = nlohmann::json::array();
    
    try {
        // Parse the raw JSON string from SearchClient
        nlohmann::json redisResponse = nlohmann::json::parse(rawResponse);
        
        if (!redisResponse.is_array() || redisResponse.empty()) {
            response["meta"]["total"] = 0;
            response["meta"]["page"] = page;
            response["meta"]["pageSize"] = limit;
            return response;
        }
        
        // First element is the total count
        int totalResults = 0;
        if (redisResponse.size() > 0 && redisResponse[0].is_number()) {
            totalResults = redisResponse[0].get<int>();
        }
        
        response["meta"]["total"] = totalResults;
        response["meta"]["page"] = page;
        response["meta"]["pageSize"] = limit;
        
        // Parse each result (skip the count, then pairs of docId and fields)
        for (size_t i = 1; i < redisResponse.size(); i += 2) {
            if (i + 1 >= redisResponse.size()) break;
            
            std::string docId = redisResponse[i].is_string() ? redisResponse[i].get<std::string>() : "";
            
            if (redisResponse[i + 1].is_array()) {
                nlohmann::json fields = redisResponse[i + 1];
                
                nlohmann::json result;
                result["url"] = "";
                result["title"] = "";
                result["snippet"] = "";
                result["score"] = 1.0;
                
                // Parse field pairs
                for (size_t j = 0; j < fields.size(); j += 2) {
                    if (j + 1 >= fields.size()) break;
                    
                    std::string fieldName = fields[j].is_string() ? fields[j].get<std::string>() : "";
                    std::string fieldValue = fields[j + 1].is_string() ? fields[j + 1].get<std::string>() : "";
                    
                    if (fieldName == "url") {
                        result["url"] = fieldValue;
                    } else if (fieldName == "title") {
                        result["title"] = fieldValue;
                    } else if (fieldName == "content") {
                        std::string snippet = fieldValue.substr(0, 200);
                        if (fieldValue.length() > 200) {
                            snippet += "...";
                        }
                        result["snippet"] = snippet;
                    } else if (fieldName == "score") {
                        try {
                            result["score"] = std::stod(fieldValue);
                        } catch (...) {
                            result["score"] = 1.0;
                        }
                    }
                }
                
                response["results"].push_back(result);
            }
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse Redis search response: " + std::string(e.what()));
        response["meta"]["total"] = 0;
        response["meta"]["page"] = page;
        response["meta"]["pageSize"] = limit;
    }
    
    return response;
} 