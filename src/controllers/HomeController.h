#pragma once
#include "../../include/routing/Controller.h"
#include "../../include/inja/inja.hpp"
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

class HomeController : public routing::Controller {
public:
    // GET /
    void index(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // GET /test
    void searchPage(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // POST /api/v2/email-subscribe
    void emailSubscribe(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // GET /crawl-request
    void crawlRequestPage(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);

private:
    std::string loadFile(const std::string& path);
    std::string renderTemplate(const std::string& templateName, const nlohmann::json& data);
};

// Route registration using macros (similar to .NET Core attributes)
ROUTE_CONTROLLER(HomeController) {
    using namespace routing;
    REGISTER_ROUTE(HttpMethod::GET, "/", index, HomeController);
    REGISTER_ROUTE(HttpMethod::GET, "/test", searchPage, HomeController);
    REGISTER_ROUTE(HttpMethod::GET, "/crawl-request", crawlRequestPage, HomeController);
    REGISTER_ROUTE(HttpMethod::GET, "/crawl-request.html", crawlRequestPage, HomeController);
    REGISTER_ROUTE(HttpMethod::POST, "/api/v2/email-subscribe", emailSubscribe, HomeController);
} 