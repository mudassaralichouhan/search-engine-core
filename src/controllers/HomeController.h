#pragma once
#include "../../include/routing/Controller.h"
#include "../../include/inja/inja.hpp"
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

class HomeController : public routing::Controller {
public:

    void addDomain(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    // GET /
    void index(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // GET /test
    void searchPage(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // POST /api/v2/email-subscribe
    void emailSubscribe(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // GET /crawl-request
    void crawlRequestPage(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // GET /crawl-request/{lang}
    void crawlRequestPageWithLang(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);

    // GET /sponsor
    void sponsorPage(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // GET /sponsor/{lang}
    void sponsorPageWithLang(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // POST /api/v2/sponsor-submit
    void sponsorSubmit(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void getSponsorPaymentAccounts(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);

private:
    std::string getAvailableLocales();
    std::string getDefaultLocale();
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
    REGISTER_ROUTE(HttpMethod::GET, "/crawl-request/*", crawlRequestPageWithLang, HomeController);
    REGISTER_ROUTE(HttpMethod::GET, "/sponsor", sponsorPage, HomeController);
    REGISTER_ROUTE(HttpMethod::GET, "/sponsor.html", sponsorPage, HomeController);
    REGISTER_ROUTE(HttpMethod::GET, "/sponsor/*", sponsorPageWithLang, HomeController);
    REGISTER_ROUTE(HttpMethod::POST, "/api/v2/email-subscribe", emailSubscribe, HomeController);
    REGISTER_ROUTE(HttpMethod::POST, "/api/v2/sponsor-submit", sponsorSubmit, HomeController);
    REGISTER_ROUTE(HttpMethod::GET, "/api/v2/sponsor-payment-accounts", getSponsorPaymentAccounts, HomeController);
} 