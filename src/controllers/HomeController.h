#pragma once
#include "../../include/routing/Controller.h"
#include <fstream>
#include <sstream>

class HomeController : public routing::Controller {
public:
    // GET /
    void index(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // GET /test
    void searchPage(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // POST /api/v2/email-subscribe
    void emailSubscribe(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);

private:
    std::string loadFile(const std::string& path);
};

// Route registration using macros (similar to .NET Core attributes)
ROUTE_CONTROLLER(HomeController) {
    using namespace routing;
    REGISTER_ROUTE(HttpMethod::GET, "/", index, HomeController);
    REGISTER_ROUTE(HttpMethod::GET, "/test", searchPage, HomeController);
    REGISTER_ROUTE(HttpMethod::POST, "/api/v2/email-subscribe", emailSubscribe, HomeController);
} 