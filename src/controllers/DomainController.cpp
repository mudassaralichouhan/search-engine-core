#include "DomainController.h"
#include "../../include/Logger.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <regex>

// Using fully qualified names to avoid conflict with json() member function

// DomainController::DomainController() {
//     // Empty constructor - use lazy initialization to avoid static initialization order fiasco
//     // Members will be initialized on first use via getDomainStorage() and getJobQueue()
// }

void DomainController::addDomain(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    std::string buffer;
    res->onData([this, res, buffer = std::move(buffer)](std::string_view data, bool last) mutable {
        buffer.append(data.data(), data.length());

        
        if (last) {
            try {
                auto jsonBody = nlohmann::json::parse(buffer);
                
                std::string domain = jsonBody.value("domain", "");
                std::string webmasterEmail = jsonBody.value("webmasterEmail", "");
                int maxPages = jsonBody.value("maxPages", 5);
                
                if (domain.empty()) {
                    badRequest(res, "Domain is required");
                    return;
                }
                
                if (!validateDomainName(domain)) {
                    badRequest(res, "Invalid domain name");
                    return;
                }
                
                if (!webmasterEmail.empty() && !validateEmail(webmasterEmail)) {
                    badRequest(res, "Invalid email address");
                    return;
                }
                
                // Check if domain already exists
                auto existingDomain = getDomainStorage()->getDomainByName(domain);
                if (existingDomain) {
                    badRequest(res, "Domain already exists");
                    return;
                }
                
                std::string domainId = getDomainStorage()->addDomain(domain, webmasterEmail, maxPages);
                
                if (domainId.empty()) {
                    serverError(res, "Failed to add domain");
                    return;
                }
                
                nlohmann::json response = {
                    {"success", true},
                    {"message", "Domain added successfully"},
                    {"data", {
                        {"domainId", domainId},
                        {"domain", domain},
                        {"webmasterEmail", webmasterEmail},
                        {"maxPages", maxPages}
                    }}
                };
                
                this->json(res, response, 201);
                
            } catch (const nlohmann::json::exception& e) {
                badRequest(res, "Invalid JSON format: " + std::string(e.what()));
            } catch (const std::exception& e) {
                serverError(res, "Error processing request: " + std::string(e.what()));
            }
        }
    });

    // CRITICAL: Always add onAborted to prevent server crashes when client disconnects
    res->onAborted([]() {
        LOG_WARNING("Client disconnected during addDomain request processing");
    });
}

void DomainController::addDomainsBulk(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    std::string buffer;
    res->onData([this, res, buffer = std::move(buffer)](std::string_view data, bool last) mutable {
        buffer.append(data.data(), data.length());
        
        if (last) {
            try {
                auto jsonBody = nlohmann::json::parse(buffer);
                
                if (!jsonBody.contains("domains") || !jsonBody["domains"].is_array()) {
                    badRequest(res, "Domains array is required");
                    return;
                }
                
                std::vector<std::pair<std::string, std::string>> domainsToAdd;
                int maxPages = 5;
                
                for (const auto& domainObj : jsonBody["domains"]) {
                    std::string domain = domainObj.value("domain", "");
                    std::string webmasterEmail = domainObj.value("webmasterEmail", "");
                    
                    if (domain.empty()) {
                        badRequest(res, "All domains must have a domain name");
                        return;
                    }
                    
                    if (!validateDomainName(domain)) {
                        badRequest(res, "Invalid domain name: " + domain);
                        return;
                    }
                    
                    if (!webmasterEmail.empty() && !validateEmail(webmasterEmail)) {
                        badRequest(res, "Invalid email address for domain: " + domain);
                        return;
                    }
                    
                    maxPages = domainObj.value("maxPages", maxPages);
                    domainsToAdd.emplace_back(domain, webmasterEmail);
                }
                
                auto domainIds = getDomainStorage()->addDomains(domainsToAdd, maxPages);
                
                if (domainIds.empty()) {
                    serverError(res, "Failed to add domains");
                    return;
                }
                
                nlohmann::json response = {
                    {"success", true},
                    {"message", "Domains added successfully"},
                    {"data", {
                        {"count", domainIds.size()},
                        {"domainIds", domainIds}
                    }}
                };
                
                this->json(res, response, 201);
                
            } catch (const nlohmann::json::exception& e) {
                badRequest(res, "Invalid JSON format: " + std::string(e.what()));
            } catch (const std::exception& e) {
                serverError(res, "Error processing request: " + std::string(e.what()));
            }
        }
    });

    // CRITICAL: Always add onAborted to prevent server crashes when client disconnects
    res->onAborted([]() {
        LOG_WARNING("Client disconnected during addDomainsBulk request processing");
    });
}

void DomainController::addDomainWithBusinessInfo(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    std::string buffer;
    res->onData([this, res, buffer = std::move(buffer)](std::string_view data, bool last) mutable {
        buffer.append(data.data(), data.length());
        
        if (last) {
            try {
                LOG_INFO("Processing addDomainWithBusinessInfo request, buffer size: " + std::to_string(buffer.size()));
                
                LOG_INFO("Step 1: Parsing JSON");
                auto jsonBody = nlohmann::json::parse(buffer);
                LOG_INFO("Step 1 completed: JSON parsed successfully");
                
                // Validate required fields
                LOG_INFO("Step 2: Extracting required fields");
                std::string websiteUrl = jsonBody.value("website_url", "");
                std::string email = jsonBody.value("email", "");
                LOG_INFO("Step 2 completed: Website URL: " + websiteUrl + ", Email: " + email);
                
                if (websiteUrl.empty()) {
                    LOG_ERROR("website_url is required");
                    badRequest(res, "website_url is required");
                    return;
                }
                
                LOG_INFO("Step 3: Validating domain name");
                if (!validateDomainName(websiteUrl)) {
                    LOG_ERROR("Invalid website URL: " + websiteUrl);
                    badRequest(res, "Invalid website URL");
                    return;
                }
                LOG_INFO("Step 3 completed: Domain name validated");
                
                LOG_INFO("Step 4: Validating email");
                if (!email.empty() && !validateEmail(email)) {
                    LOG_ERROR("Invalid email address: " + email);
                    badRequest(res, "Invalid email address");
                    return;
                }
                LOG_INFO("Step 4 completed: Email validated");
                
                // Check if domain already exists
                LOG_INFO("Step 5: Checking if domain exists: " + websiteUrl);
                auto existingDomain = getDomainStorage()->getDomainByName(websiteUrl);
                LOG_INFO("Step 5 completed: Domain existence check done");
                if (existingDomain) {
                    LOG_ERROR("Domain already exists: " + websiteUrl);
                    badRequest(res, "Domain already exists");
                    return;
                }
                
                LOG_INFO("Step 6: Getting maxPages parameter");
                int maxPages = jsonBody.value("maxPages", 5);
                LOG_INFO("Step 6 completed: Adding domain with maxPages: " + std::to_string(maxPages));
                
                // Use the new method to add domain with business info
                LOG_INFO("Step 7: Calling addDomainWithBusinessInfo");
                std::string domainId = getDomainStorage()->addDomainWithBusinessInfo(jsonBody, maxPages);
                LOG_INFO("Step 7 completed: addDomainWithBusinessInfo returned: " + domainId);
                
                if (domainId.empty()) {
                    LOG_ERROR("Failed to add domain with business information");
                    serverError(res, "Failed to add domain with business information");
                    return;
                }
                
                LOG_INFO("Domain added successfully with ID: " + domainId);
                
                // Get the created domain to return complete information
                auto createdDomain = getDomainStorage()->getDomain(domainId);
                
                if (!createdDomain) {
                    LOG_ERROR("Failed to retrieve created domain with ID: " + domainId);
                    serverError(res, "Failed to retrieve created domain");
                    return;
                }
                
                nlohmann::json response = {
                    {"success", true},
                    {"message", "Domain with business information added successfully"},
                    {"data", createdDomain->toJson()}
                };
                
                LOG_INFO("Sending success response");
                this->json(res, response, 201);
                
            } catch (const nlohmann::json::exception& e) {
                LOG_ERROR("JSON parsing error: " + std::string(e.what()));
                badRequest(res, "Invalid JSON format: " + std::string(e.what()));
            } catch (const std::exception& e) {
                LOG_ERROR("Exception in addDomainWithBusinessInfo: " + std::string(e.what()));
                serverError(res, "Error processing request: " + std::string(e.what()));
            } catch (...) {
                LOG_ERROR("Unknown exception in addDomainWithBusinessInfo");
                serverError(res, "Unknown error occurred");
            }
        }
    });

    // CRITICAL: Always add onAborted to prevent server crashes when client disconnects
    res->onAborted([]() {
        LOG_WARNING("Client disconnected during addDomainWithBusinessInfo request processing");
    });
}

void DomainController::testPost(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    try {
        LOG_INFO("TEST GET endpoint called - testing lazy initialization");
        
        // Trigger lazy initialization 
        auto storage = getDomainStorage();
        auto queue = getJobQueue();
        
        nlohmann::json response = {
            {"success", true}, 
            {"message", "DomainController lazy initialization test!"},
            {"domainStorage", storage ? "lazy initialized successfully" : "failed"},
            {"jobQueue", queue ? "lazy initialized successfully" : "failed"}
        };
        this->json(res, response);
        LOG_INFO("Lazy initialization test completed");
    } catch (const std::exception& e) {
        LOG_ERROR("Error in lazy initialization test: " + std::string(e.what()));
        serverError(res, "Lazy initialization test failed: " + std::string(e.what()));
    }
}

void DomainController::startBulkCrawl(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    std::string buffer;
    res->onData([this, res, buffer = std::move(buffer)](std::string_view data, bool last) mutable {
        buffer.append(data.data(), data.length());
        
        if (last) {
            try {
                auto jsonBody = nlohmann::json::parse(buffer);
                
                int maxConcurrent = jsonBody.value("maxConcurrent", 10);
                bool emailNotifications = jsonBody.value("emailNotifications", true);
                
                // Get domains ready for crawling
                auto domains = getDomainStorage()->getDomainsReadyForCrawling(1000);
                
                if (domains.empty()) {
                    badRequest(res, "No domains ready for crawling");
                    return;
                }
                
                // Create domain crawl jobs
                std::vector<DomainJob> domainJobs;
                for (const auto& domain : domains) {
                    DomainJob job;
                    job.domain = domain.domain;
                    job.webmasterEmail = domain.webmasterEmail;
                    job.maxPages = domain.maxPages;
                    job.sessionId = "";  // Will be set by crawler
                    job.createdAt = std::chrono::system_clock::now();
                    
                    domainJobs.push_back(job);
                    
                    // Update domain status to crawling
                    getDomainStorage()->updateDomainStatus(domain.id, DomainStatus::CRAWLING);
                }
                
                // Add jobs to queue
                auto jobIds = getJobQueue()->addBulkDomainCrawlJobs(domainJobs);
                
                nlohmann::json response = {
                    {"success", true},
                    {"message", "Bulk crawl started successfully"},
                    {"data", {
                        {"domainsQueued", domains.size()},
                        {"jobIds", jobIds},
                        {"maxConcurrent", maxConcurrent},
                        {"emailNotifications", emailNotifications}
                    }}
                };
                
                this->json(res, response);
                
            } catch (const nlohmann::json::exception& e) {
                badRequest(res, "Invalid JSON format: " + std::string(e.what()));
            } catch (const std::exception& e) {
                serverError(res, "Error starting bulk crawl: " + std::string(e.what()));
            }
        }
    });

    // CRITICAL: Always add onAborted to prevent server crashes when client disconnects
    res->onAborted([]() {
        LOG_WARNING("Client disconnected during startBulkCrawl request processing");
    });
}

void DomainController::getDomainStats(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    try {
        auto stats = getDomainStorage()->getStats();
        
        nlohmann::json response = {
            {"success", true},
            {"data", {
                {"totalDomains", stats.totalDomains},
                {"pendingDomains", stats.pendingDomains},
                {"crawlingDomains", stats.crawlingDomains},
                {"completedDomains", stats.completedDomains},
                {"failedDomains", stats.failedDomains},
                {"emailsSent", stats.emailsSent}
            }}
        };
        
        this->json(res, response);
        
    } catch (const std::exception& e) {
        serverError(res, "Error getting domain stats: " + std::string(e.what()));
    }
}

void DomainController::getQueueStats(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    try {
        auto stats = getJobQueue()->getStats();
        
        nlohmann::json response = {
            {"success", true},
            {"data", {
                {"pending", stats.pending},
                {"processing", stats.processing},
                {"completed", stats.completed},
                {"failed", stats.failed},
                {"total", stats.total}
            }}
        };
        
        this->json(res, response);
        
    } catch (const std::exception& e) {
        serverError(res, "Error getting queue stats: " + std::string(e.what()));
    }
}

void DomainController::getDomain(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    try {
        std::string domainId = std::string(req->getParameter(0));
        
        if (domainId.empty()) {
            badRequest(res, "Domain ID is required");
            return;
        }
        
        auto domain = getDomainStorage()->getDomain(domainId);
        
        if (!domain) {
            notFound(res, "Domain not found");
            return;
        }
        
        nlohmann::json response = {
            {"success", true},
            {"data", domain->toJson()}
        };
        
        this->json(res, response);
        
    } catch (const std::exception& e) {
        serverError(res, "Error getting domain: " + std::string(e.what()));
    }
}

void DomainController::getDomains(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    try {
        LOG_INFO("getDomains API called");
        
        // Parse query parameters
        std::string statusParam = std::string(req->getQuery("status"));
        std::string limitParam = std::string(req->getQuery("limit"));
        std::string offsetParam = std::string(req->getQuery("offset"));
        
        LOG_INFO("Query parameters - status: '" + statusParam + "', limit: '" + limitParam + "', offset: '" + offsetParam + "'");
        
        int limit = limitParam.empty() ? 100 : std::stoi(limitParam);
        int offset = offsetParam.empty() ? 0 : std::stoi(offsetParam);
        
        LOG_INFO("Parsed parameters - limit: " + std::to_string(limit) + ", offset: " + std::to_string(offset));
        
        std::vector<Domain> domains;
        
        if (!statusParam.empty()) {
            LOG_INFO("Getting domains by status: " + statusParam);
            DomainStatus status = static_cast<DomainStatus>(std::stoi(statusParam));
            domains = getDomainStorage()->getDomainsByStatus(status, limit, offset);
        } else {
            LOG_INFO("Getting all domains");
            domains = getDomainStorage()->getAllDomains(limit, offset);
        }
        
        LOG_INFO("Retrieved " + std::to_string(domains.size()) + " domains from storage");
        
        nlohmann::json domainsJson = nlohmann::json::array();
        for (size_t i = 0; i < domains.size(); ++i) {
            try {
                LOG_DEBUG("Converting domain " + std::to_string(i) + " to JSON: " + domains[i].domain);
                domainsJson.push_back(domains[i].toJson());
                LOG_DEBUG("Successfully converted domain " + std::to_string(i) + " to JSON");
            } catch (const std::exception& e) {
                LOG_ERROR("Error converting domain " + std::to_string(i) + " to JSON: " + std::string(e.what()));
                throw;
            }
        }
        
        LOG_INFO("Successfully converted all domains to JSON");
        
        nlohmann::json response = {
            {"success", true},
            {"data", {
                {"domains", domainsJson},
                {"count", domains.size()},
                {"limit", limit},
                {"offset", offset}
            }}
        };
        
        LOG_INFO("Sending response with " + std::to_string(domains.size()) + " domains");
        this->json(res, response);
        LOG_INFO("getDomains API completed successfully");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error in getDomains: " + std::string(e.what()));
        serverError(res, "Error getting domains: " + std::string(e.what()));
    } catch (...) {
        LOG_ERROR("Unknown error in getDomains");
        serverError(res, "Unknown error occurred while getting domains");
    }
}

void DomainController::getSystemHealth(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    try {
        nlohmann::json response = {
            {"success", true},
            {"data", {
                {"status", "healthy"},
                {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count()},
                {"components", {
                    {"domainStorage", "up"},
                    {"jobQueue", getJobQueue()->isRunning() ? "up" : "down"},
                    {"redis", "up"},  // TODO: Add actual health check
                    {"mongodb", "up"}  // TODO: Add actual health check
                }}
            }}
        };
        
        this->json(res, response);
        
    } catch (const std::exception& e) {
        serverError(res, "Error checking system health: " + std::string(e.what()));
    }
}

// Helper methods
void DomainController::json(uWS::HttpResponse<false>* res, const nlohmann::json& data, int statusCode) {
    std::string statusText = (statusCode == 200) ? "200 OK" : 
                            (statusCode == 201) ? "201 Created" : 
                            (statusCode == 400) ? "400 Bad Request" :
                            (statusCode == 404) ? "404 Not Found" :
                            (statusCode == 500) ? "500 Internal Server Error" : 
                            std::to_string(statusCode);
    
    res->writeStatus(statusText);
    res->writeHeader("Content-Type", "application/json");
    res->writeHeader("Access-Control-Allow-Origin", "*");
    res->end(data.dump());
}

void DomainController::badRequest(uWS::HttpResponse<false>* res, const std::string& message) {
    nlohmann::json response = {
        {"success", false},
        {"message", message}
    };
    this->json(res, response, 400);
}

void DomainController::serverError(uWS::HttpResponse<false>* res, const std::string& message) {
    LOG_ERROR("Server error: " + message);
    nlohmann::json response = {
        {"success", false},
        {"message", "Internal server error"}
    };
    this->json(res, response, 500);
}

void DomainController::notFound(uWS::HttpResponse<false>* res, const std::string& message) {
    nlohmann::json response = {
        {"success", false},
        {"message", message}
    };
    this->json(res, response, 404);
}

bool DomainController::validateDomainName(const std::string& domain) {
    // Basic domain validation regex
    std::regex domainRegex(R"(^(?:[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?\.)*[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?$)");
    return std::regex_match(domain, domainRegex) && domain.length() <= 253;
}

bool DomainController::validateEmail(const std::string& email) {
    // Basic email validation regex
    std::regex emailRegex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    return std::regex_match(email, emailRegex) && email.length() <= 254;
}

// Additional implementation for remaining methods
void DomainController::startDomainCrawl(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    // Implementation for single domain crawl
    badRequest(res, "Method not yet implemented");
}

void DomainController::stopCrawl(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    // Implementation for stopping crawl
    badRequest(res, "Method not yet implemented");
}

// Lazy initialization helpers
DomainStorage* DomainController::getDomainStorage() const {
    if (!domainStorage_) {
        try {
            LOG_INFO("Lazy initializing DomainStorage");
            domainStorage_ = std::make_unique<DomainStorage>();
            LOG_INFO("DomainStorage lazy initialization successful");
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to lazy initialize DomainStorage: " + std::string(e.what()));
            throw;
        }
    }
    return domainStorage_.get();
}

JobQueue* DomainController::getJobQueue() const {
    if (!jobQueue_) {
        try {
            LOG_INFO("Lazy initializing JobQueue");
            jobQueue_ = std::make_unique<JobQueue>();
            LOG_INFO("JobQueue lazy initialization successful");
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to lazy initialize JobQueue: " + std::string(e.what()));
            throw;
        }
    }
    return jobQueue_.get();
}

void DomainController::getJobStatus(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    // Implementation for job status
    badRequest(res, "Method not yet implemented");
}

void DomainController::getJobQueue(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    // Implementation for job queue listing
    badRequest(res, "Method not yet implemented");
}

void DomainController::retryFailedJobs(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    // Implementation for retrying failed jobs
    badRequest(res, "Method not yet implemented");
}

void DomainController::sendTestEmail(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    // Implementation for test email
    badRequest(res, "Method not yet implemented");
}

void DomainController::resendEmails(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    // Implementation for resending emails
    badRequest(res, "Method not yet implemented");
}

void DomainController::importFromCSV(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    // Implementation for CSV import
    badRequest(res, "Method not yet implemented");
}

void DomainController::exportToCSV(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    // Implementation for CSV export
    badRequest(res, "Method not yet implemented");
}

void DomainController::updateDomain(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    // Implementation for domain update
    badRequest(res, "Method not yet implemented");
}

void DomainController::deleteDomain(uWS::HttpResponse<false>* res, uWS::HttpRequest* req) {
    // Implementation for domain deletion
    badRequest(res, "Method not yet implemented");
}
