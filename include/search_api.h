#pragma once

#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>
#include <string>
#include <map>

namespace search_api {
    // Function to handle search requests
    void handleSearch(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // Helper function to parse and validate pagination parameters
    struct PaginationParams {
        int page = 1;
        int limit = 10;
        bool valid = true;
        std::string error;
    };
    
    PaginationParams parsePaginationParams(const std::map<std::string, std::string>& params);
} 