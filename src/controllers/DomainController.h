#pragma once

#include <memory>
#include "../../include/routing/Controller.h"
#include "../../include/storage/DomainStorage.h"
#include "../../include/job_queue/JobQueue.h"

class DomainController : public routing::Controller {
public:
  //  DomainController();
   // ~DomainController() = default;
    
    // Domain management endpoints
    void addDomain(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void addDomainsBulk(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void addDomainWithBusinessInfo(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void testPost(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void simplePostTest(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void getDomain(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void getDomains(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void updateDomain(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void deleteDomain(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // Crawling operations
    void startBulkCrawl(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void startDomainCrawl(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void stopCrawl(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // Job management
    void getJobStatus(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void getJobQueue(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void retryFailedJobs(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // Statistics and monitoring
    void getDomainStats(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void getQueueStats(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void getSystemHealth(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // Email operations
    void sendTestEmail(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void resendEmails(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // CSV import/export
    void importFromCSV(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void exportToCSV(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
private:
    mutable std::unique_ptr<DomainStorage> domainStorage_;
    mutable std::unique_ptr<JobQueue> jobQueue_;
    
    // Lazy initialization helpers
    DomainStorage* getDomainStorage() const;
    JobQueue* getJobQueue() const;
    
    // Helper methods
    void json(uWS::HttpResponse<false>* res, const nlohmann::json& data, int statusCode = 200);
    void badRequest(uWS::HttpResponse<false>* res, const std::string& message);
    void serverError(uWS::HttpResponse<false>* res, const std::string& message);
    void notFound(uWS::HttpResponse<false>* res, const std::string& message);
    
    std::string readRequestBody(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    bool validateDomainName(const std::string& domain);
    bool validateEmail(const std::string& email);
};

// Route registration using macros
ROUTE_CONTROLLER(DomainController) {
    using namespace routing;
    // All DomainController routes - now with lazy initialization to fix static initialization bug
    REGISTER_ROUTE(HttpMethod::POST, "/api/v2/domains", addDomain, DomainController);
    REGISTER_ROUTE(HttpMethod::POST, "/api/v2/domains/bulk", addDomainsBulk, DomainController);
    REGISTER_ROUTE(HttpMethod::POST, "/api/v2/domains/business-info", addDomainWithBusinessInfo, DomainController);
    REGISTER_ROUTE(HttpMethod::GET, "/api/v2/domains/{id}", getDomain, DomainController);
    REGISTER_ROUTE(HttpMethod::GET, "/api/v2/domains", getDomains, DomainController);
    REGISTER_ROUTE(HttpMethod::PUT, "/api/v2/domains/{id}", updateDomain, DomainController);
    REGISTER_ROUTE(HttpMethod::DELETE, "/api/v2/domains/{id}", deleteDomain, DomainController);
    REGISTER_ROUTE(HttpMethod::POST, "/api/v2/crawl/bulk", startBulkCrawl, DomainController);
    REGISTER_ROUTE(HttpMethod::POST, "/api/v2/crawl/{id}", startDomainCrawl, DomainController);
    REGISTER_ROUTE(HttpMethod::POST, "/api/v2/crawl/stop", stopCrawl, DomainController);
    REGISTER_ROUTE(HttpMethod::GET, "/api/v2/jobs/{id}", getJobStatus, DomainController);
    REGISTER_ROUTE(HttpMethod::GET, "/api/v2/jobs", getJobQueue, DomainController);
    REGISTER_ROUTE(HttpMethod::POST, "/api/v2/jobs/retry", retryFailedJobs, DomainController);
    REGISTER_ROUTE(HttpMethod::GET, "/api/v2/stats/domains", getDomainStats, DomainController);
    REGISTER_ROUTE(HttpMethod::GET, "/api/v2/stats/queue", getQueueStats, DomainController);
    REGISTER_ROUTE(HttpMethod::GET, "/api/v2/health", getSystemHealth, DomainController);
    REGISTER_ROUTE(HttpMethod::POST, "/api/v2/email/test", sendTestEmail, DomainController);
    REGISTER_ROUTE(HttpMethod::POST, "/api/v2/email/resend", resendEmails, DomainController);
    REGISTER_ROUTE(HttpMethod::POST, "/api/v2/import/csv", importFromCSV, DomainController);
    REGISTER_ROUTE(HttpMethod::GET, "/api/v2/export/csv", exportToCSV, DomainController);
}
