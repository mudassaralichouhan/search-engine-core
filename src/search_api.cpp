#include "../include/search_api.h"
#include "../include/api.h"
#include "../include/Logger.h"
#include <sstream>

using json = nlohmann::json;

namespace search_api {

PaginationParams parsePaginationParams(const std::map<std::string, std::string>& params) {
    PaginationParams result;
    
    // Parse page parameter
    auto pageIt = params.find("page");
    if (pageIt != params.end()) {
        try {
            result.page = std::stoi(pageIt->second);
            if (result.page < 1 || result.page > 1000) {
                result.valid = false;
                result.error = "Page must be between 1 and 1000";
                return result;
            }
        } catch (const std::exception& e) {
            result.valid = false;
            result.error = "Invalid page parameter: must be an integer";
            return result;
        }
    }
    
    // Parse limit parameter
    auto limitIt = params.find("limit");
    if (limitIt != params.end()) {
        try {
            result.limit = std::stoi(limitIt->second);
            if (result.limit < 1 || result.limit > 100) {
                result.valid = false;
                result.error = "Limit must be between 1 and 100";
                return result;
            }
        } catch (const std::exception& e) {
            result.valid = false;
            result.error = "Invalid limit parameter: must be an integer";
            return result;
        }
    }
    
    return result;
}

void handleSearch(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    LOG_INFO("Handling search request");
    
    // Parse query string
    std::string queryString = std::string(req->getQuery());
    auto params = api::parseQueryString(queryString);
    
    // Check for required 'q' parameter
    auto qIt = params.find("q");
    if (qIt == params.end() || qIt->second.empty()) {
        // Return 400 error for missing 'q' parameter
        json errorResponse = {
            {"error", {
                {"code", "INVALID_REQUEST"},
                {"message", "Invalid request parameters"},
                {"details", {
                    {"q", "Query parameter is required"}
                }}
            }}
        };
        
        res->writeStatus("400 Bad Request")
           ->writeHeader("Content-Type", "application/json")
           ->end(errorResponse.dump());
        
        LOG_WARNING("Search request rejected: missing 'q' parameter");
        return;
    }
    
    // Parse pagination parameters
    auto paginationParams = parsePaginationParams(params);
    if (!paginationParams.valid) {
        // Return 400 error for invalid pagination parameters
        json errorResponse = {
            {"error", {
                {"code", "INVALID_REQUEST"},
                {"message", "Invalid request parameters"},
                {"details", {
                    {"page", paginationParams.error}
                }}
            }}
        };
        
        res->writeStatus("400 Bad Request")
           ->writeHeader("Content-Type", "application/json")
           ->end(errorResponse.dump());
        
        LOG_WARNING("Search request rejected: " + paginationParams.error);
        return;
    }
    
    // Parse domain_filter if present
    std::vector<std::string> domainFilter;
    auto domainIt = params.find("domain_filter");
    if (domainIt != params.end() && !domainIt->second.empty()) {
        std::stringstream ss(domainIt->second);
        std::string domain;
        while (std::getline(ss, domain, ',')) {
            domainFilter.push_back(domain);
        }
    }
    
    // For now, return the hardcoded example JSON from Issue #1
    json response = {
        {"meta", {
            {"total", 123},
            {"page", paginationParams.page},
            {"pageSize", paginationParams.limit}
        }},
        {"results", json::array({
            {
                {"url", "https://example.com/page"},
                {"title", "Example Page Title"},
                {"snippet", "This is a snippet of the content with <em>highlighted</em> search terms..."},
                {"score", 0.987},
                {"timestamp", "2024-01-15T10:30:00Z"}
            },
            {
                {"url", "https://docs.example.com/guide"},
                {"title", "Getting Started Guide"},
                {"snippet", "Learn how to get started with our <em>search</em> API..."},
                {"score", 0.845},
                {"timestamp", "2024-01-14T08:15:00Z"}
            },
            {
                {"url", "https://blog.example.com/search-tips"},
                {"title", "Advanced Search Tips and Tricks"},
                {"snippet", "Discover advanced <em>search</em> techniques to find exactly what you need..."},
                {"score", 0.723},
                {"timestamp", "2024-01-13T14:22:00Z"}
            }
        })}
    };
    
    // Log successful request
    LOG_INFO("Search request successful: q=" + qIt->second + 
             ", page=" + std::to_string(paginationParams.page) + 
             ", limit=" + std::to_string(paginationParams.limit) +
             ", domains=" + std::to_string(domainFilter.size()));
    
    // Return 200 OK with the stub response
    res->writeStatus("200 OK")
       ->writeHeader("Content-Type", "application/json")
       ->writeHeader("Access-Control-Allow-Origin", "*")  // CORS support
       ->end(response.dump());
}

} // namespace search_api 