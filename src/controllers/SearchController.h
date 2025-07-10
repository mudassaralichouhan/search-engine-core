#pragma once
#include "../../include/routing/Controller.h"
#include "../../include/search_core/SearchClient.hpp"
#include <memory>
#include <nlohmann/json.hpp>

class SearchController : public routing::Controller {
public:
    SearchController();
    
    // Search functionality
    void search(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // Crawl management
    void addSiteToCrawl(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void getCrawlStatus(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);

private:
    nlohmann::json parseRedisSearchResponse(const std::string& rawResponse, int page, int limit);
};

// Route registration
ROUTE_CONTROLLER(SearchController) {
    using namespace routing;
    REGISTER_ROUTE(HttpMethod::GET, "/api/search", search, SearchController);
    REGISTER_ROUTE(HttpMethod::POST, "/api/crawl/add-site", addSiteToCrawl, SearchController);
    REGISTER_ROUTE(HttpMethod::GET, "/api/crawl/status", getCrawlStatus, SearchController);
} 