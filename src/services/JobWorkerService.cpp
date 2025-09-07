#include "JobWorkerService.h"
#include "../../include/Logger.h"
#include "../crawler/CrawlerManager.h"
#include "../../include/search_engine/storage/ContentStorage.h"

JobWorkerService::JobWorkerService(int workerCount) : workerCount_(workerCount) {
    LOG_INFO("JobWorkerService created with " + std::to_string(workerCount) + " workers");
}

JobWorkerService::~JobWorkerService() {
    stop();
}

bool JobWorkerService::start() {
    if (running_) {
        LOG_WARNING("JobWorkerService already running");
        return true;
    }
    
    LOG_INFO("Starting JobWorkerService...");
    
    if (!initializeServices()) {
        LOG_ERROR("Failed to initialize services");
        return false;
    }
    
    // Setup custom job handlers
    setupCustomJobHandlers();
    
    // Start job queue workers
    jobQueue_->startWorkers(workerCount_);
    
    // Start monitoring thread
    shouldStop_ = false;
    monitoringThread_ = std::thread(&JobWorkerService::monitoringLoop, this);
    
    running_ = true;
    startedAt_ = std::chrono::system_clock::now();
    
    LOG_INFO("JobWorkerService started successfully");
    return true;
}

void JobWorkerService::stop() {
    if (!running_) {
        return;
    }
    
    LOG_INFO("Stopping JobWorkerService...");
    
    running_ = false;
    shouldStop_ = true;
    
    // Stop job queue workers
    if (jobQueue_) {
        jobQueue_->stopWorkers();
    }
    
    // Stop monitoring thread
    if (monitoringThread_.joinable()) {
        monitoringThread_.join();
    }
    
    LOG_INFO("JobWorkerService stopped");
}

bool JobWorkerService::initializeServices() {
    try {
        // Initialize domain storage
        domainStorage_ = std::make_unique<DomainStorage>();
        LOG_INFO("DomainStorage initialized");
        
        // Initialize job queue
        const char* redisUri = std::getenv("SEARCH_REDIS_URI");
        std::string connectionString = redisUri ? redisUri : "tcp://redis:6379";
        jobQueue_ = std::make_unique<JobQueue>(connectionString);
        LOG_INFO("JobQueue initialized");
        
        // Initialize email service
        EmailConfig emailConfig;
        emailService_ = std::make_unique<EmailService>(emailConfig);
        LOG_INFO("EmailService initialized");
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize services: " + std::string(e.what()));
        return false;
    }
}

void JobWorkerService::setupCustomJobHandlers() {
    // Override default handlers with our enhanced versions
    jobQueue_->setJobHandler(JobType::CRAWL_DOMAIN, 
        [this](const Job& job) { return handleCrawlDomainJob(job); });
    
    jobQueue_->setJobHandler(JobType::SEND_EMAIL,
        [this](const Job& job) { return handleEmailJob(job); });
    
    LOG_INFO("Custom job handlers configured");
}

bool JobWorkerService::handleCrawlDomainJob(const Job& job) {
    try {
        auto domainJob = DomainJob::fromJson(job.data);
        
        LOG_INFO("Processing crawl job for domain: " + domainJob.domain);
        
        // Update domain status
        auto domain = domainStorage_->getDomainByName(domainJob.domain);
        if (!domain) {
            LOG_ERROR("Domain not found: " + domainJob.domain);
            return false;
        }
        
        domainStorage_->updateDomainStatus(domain->id, DomainStatus::CRAWLING);
        
        // Create crawl configuration
        CrawlConfig config;
        config.maxPages = domainJob.maxPages;
        config.maxDepth = 3;
        config.politenessDelay = std::chrono::milliseconds(1000);
        config.respectRobotsTxt = true;
        config.restrictToSeedDomain = true;
        config.requestTimeout = std::chrono::milliseconds(30000);
        
        // Initialize storage for crawl results
        auto storage = std::make_shared<search_engine::storage::ContentStorage>();
        
        // Create crawler manager
        CrawlerManager crawlerManager(storage);
        
        // Start crawl
        std::string sessionId = crawlerManager.startCrawl("https://" + domainJob.domain, config, false);
        
        if (sessionId.empty()) {
            LOG_ERROR("Failed to start crawl for domain: " + domainJob.domain);
            domainStorage_->updateDomainStatus(domain->id, DomainStatus::FAILED);
            return false;
        }
        
        // Update domain with crawl info
        domainStorage_->updateDomainCrawlInfo(domain->id, sessionId, job.id, 0);
        
        // Wait for crawl to complete (with timeout)
        auto startTime = std::chrono::system_clock::now();
        auto timeout = std::chrono::minutes(15); // 15 minute timeout per domain
        
        while (true) {
            auto status = crawlerManager.getCrawlStatus(sessionId);
            if (status == "completed") {
                break;
            } else if (status == "failed") {
                LOG_ERROR("Crawl failed for domain: " + domainJob.domain);
                domainStorage_->updateDomainStatus(domain->id, DomainStatus::FAILED);
                return false;
            }
            
            // Check timeout
            if (std::chrono::system_clock::now() - startTime > timeout) {
                LOG_ERROR("Crawl timeout for domain: " + domainJob.domain);
                crawlerManager.stopCrawl(sessionId);
                domainStorage_->updateDomainStatus(domain->id, DomainStatus::FAILED);
                return false;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        
        // Get crawl results
        auto results = crawlerManager.getCrawlResults(sessionId);
        int pagesCrawled = static_cast<int>(results.size());
        
        LOG_INFO("Crawl completed for domain: " + domainJob.domain + ", pages crawled: " + std::to_string(pagesCrawled));
        
        // Update domain with final results
        domainStorage_->updateDomainCrawlInfo(domain->id, sessionId, job.id, pagesCrawled);
        domainStorage_->updateDomainStatus(domain->id, DomainStatus::COMPLETED);
        
        // Schedule email job if webmaster email is provided
        if (!domainJob.webmasterEmail.empty()) {
            EmailJob emailJob;
            emailJob.to = domainJob.webmasterEmail;
            emailJob.subject = "Your website has been crawled by our search engine";
            emailJob.templateName = "webmaster_notification";
            emailJob.templateData = json{
                {"domain", domainJob.domain},
                {"pagesCrawled", pagesCrawled},
                {"crawlDate", std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count()}
            };
            emailJob.domain = domainJob.domain;
            
            std::string emailJobId = jobQueue_->addEmailJob(emailJob);
            if (!emailJobId.empty()) {
                domainStorage_->updateDomainEmailInfo(domain->id, emailJobId);
            }
        }
        
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error processing crawl domain job: " + std::string(e.what()));
        return false;
    }
}

bool JobWorkerService::handleEmailJob(const Job& job) {
    try {
        auto emailJob = EmailJob::fromJson(job.data);
        
        LOG_INFO("Processing email job for: " + emailJob.to + " (domain: " + emailJob.domain + ")");
        
        // Send email using email service
        auto result = emailService_->sendTemplateEmail(emailJob.to, emailJob.templateName, emailJob.templateData);
        
        if (result.success) {
            LOG_INFO("Email sent successfully to: " + emailJob.to + " (Message ID: " + result.messageId + ")");
            
            // Update domain status to email sent
            auto domain = domainStorage_->getDomainByName(emailJob.domain);
            if (domain) {
                domainStorage_->updateDomainStatus(domain->id, DomainStatus::EMAIL_SENT);
            }
            
            return true;
        } else {
            LOG_ERROR("Failed to send email to: " + emailJob.to + " - " + result.message);
            return false;
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error processing email job: " + std::string(e.what()));
        return false;
    }
}

void JobWorkerService::monitoringLoop() {
    LOG_INFO("Monitoring thread started");
    
    while (!shouldStop_) {
        try {
            // Log periodic statistics
            auto stats = getStats();
            auto queueStats = jobQueue_->getStats();
            
            LOG_DEBUG("JobWorkerService Stats - Running: " + std::string(stats.running ? "Yes" : "No") + 
                     ", Workers: " + std::to_string(stats.workerCount) +
                     ", Queue Pending: " + std::to_string(queueStats.pending) +
                     ", Processing: " + std::to_string(queueStats.processing) +
                     ", Completed: " + std::to_string(queueStats.completed) +
                     ", Failed: " + std::to_string(queueStats.failed));
            
            // Sleep for 30 seconds before next check
            for (int i = 0; i < 30 && !shouldStop_; ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR("Error in monitoring loop: " + std::string(e.what()));
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    
    LOG_INFO("Monitoring thread stopped");
}

void JobWorkerService::setWorkerCount(int count) {
    if (count <= 0 || count > 50) {
        LOG_ERROR("Invalid worker count: " + std::to_string(count) + ". Must be between 1 and 50");
        return;
    }
    
    workerCount_ = count;
    LOG_INFO("Worker count set to: " + std::to_string(count));
    
    if (running_ && jobQueue_) {
        // Restart workers with new count
        jobQueue_->stopWorkers();
        jobQueue_->startWorkers(workerCount_);
        LOG_INFO("Workers restarted with new count: " + std::to_string(count));
    }
}

void JobWorkerService::setEmailConfig(const EmailConfig& config) {
    if (emailService_) {
        emailService_->updateConfig(config);
        LOG_INFO("Email configuration updated");
    } else {
        LOG_WARNING("Email service not initialized, configuration not updated");
    }
}

JobWorkerService::ServiceStats JobWorkerService::getStats() const {
    ServiceStats stats;
    stats.running = running_;
    stats.workerCount = workerCount_;
    stats.startedAt = startedAt_;
    
    if (running_) {
        stats.uptime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - startedAt_);
    } else {
        stats.uptime = std::chrono::seconds(0);
    }
    
    if (jobQueue_) {
        auto queueStats = jobQueue_->getStats();
        stats.activeJobs = queueStats.processing;
        stats.completedJobs = queueStats.completed;
        stats.failedJobs = queueStats.failed;
    } else {
        stats.activeJobs = 0;
        stats.completedJobs = 0;
        stats.failedJobs = 0;
    }
    
    return stats;
}

bool JobWorkerService::healthCheck() const {
    try {
        if (!running_) {
            return false;
        }
        
        if (!jobQueue_ || !jobQueue_->isRunning()) {
            return false;
        }
        
        if (!domainStorage_) {
            return false;
        }
        
        if (!emailService_) {
            return false;
        }
        
        // Additional health checks could be added here
        // e.g., Redis connectivity, MongoDB connectivity
        
        return true;
    } catch (...) {
        return false;
    }
}
