#include "../../include/job_queue/JobQueue.h"
#include "../../include/search_engine/storage/ContentStorage.h"
#include "../crawler/CrawlerManager.h"
#include "../../src/storage/EmailService.h"
#include <uuid/uuid.h>
#include <random>
#include <sstream>
#include <iomanip>

// Redis keys
const std::string JobQueue::QUEUE_PENDING = "job_queue:pending";
const std::string JobQueue::QUEUE_PROCESSING = "job_queue:processing";
const std::string JobQueue::QUEUE_COMPLETED = "job_queue:completed";
const std::string JobQueue::QUEUE_FAILED = "job_queue:failed";
const std::string JobQueue::JOB_DATA_PREFIX = "job_data:";
const std::string JobQueue::STATS_KEY = "job_queue:stats";

// Job serialization
json Job::toJson() const {
    return json{
        {"id", id},
        {"type", static_cast<int>(type)},
        {"status", static_cast<int>(status)},
        {"data", data},
        {"attempts", attempts},
        {"maxAttempts", maxAttempts},
        {"createdAt", std::chrono::duration_cast<std::chrono::seconds>(createdAt.time_since_epoch()).count()},
        {"scheduledAt", std::chrono::duration_cast<std::chrono::seconds>(scheduledAt.time_since_epoch()).count()},
        {"completedAt", std::chrono::duration_cast<std::chrono::seconds>(completedAt.time_since_epoch()).count()},
        {"errorMessage", errorMessage}
    };
}

Job Job::fromJson(const json& j) {
    Job job;
    job.id = j["id"];
    job.type = static_cast<JobType>(j["type"]);
    job.status = static_cast<JobStatus>(j["status"]);
    job.data = j["data"];
    job.attempts = j["attempts"];
    job.maxAttempts = j["maxAttempts"];
    job.createdAt = std::chrono::system_clock::from_time_t(j["createdAt"]);
    job.scheduledAt = std::chrono::system_clock::from_time_t(j["scheduledAt"]);
    job.completedAt = std::chrono::system_clock::from_time_t(j["completedAt"]);
    job.errorMessage = j["errorMessage"];
    return job;
}

// DomainJob serialization
json DomainJob::toJson() const {
    return json{
        {"domain", domain},
        {"webmasterEmail", webmasterEmail},
        {"maxPages", maxPages},
        {"sessionId", sessionId},
        {"createdAt", std::chrono::duration_cast<std::chrono::seconds>(createdAt.time_since_epoch()).count()}
    };
}

DomainJob DomainJob::fromJson(const json& j) {
    DomainJob job;
    job.domain = j["domain"];
    job.webmasterEmail = j["webmasterEmail"];
    job.maxPages = j["maxPages"];
    job.sessionId = j["sessionId"];
    job.createdAt = std::chrono::system_clock::from_time_t(j["createdAt"]);
    return job;
}

// EmailJob serialization  
json EmailJob::toJson() const {
    return json{
        {"to", to},
        {"subject", subject},
        {"templateName", templateName},
        {"templateData", templateData},
        {"domain", domain}
    };
}

EmailJob EmailJob::fromJson(const json& j) {
    EmailJob job;
    job.to = j["to"];
    job.subject = j["subject"];
    job.templateName = j["templateName"];
    job.templateData = j["templateData"];
    job.domain = j["domain"];
    return job;
}

JobQueue::JobQueue(const std::string& redisUri) {
    try {
        // Parse Redis URI
        sw::redis::ConnectionOptions opts;
        if (redisUri.find("tcp://") == 0) {
            std::string hostPort = redisUri.substr(6);
            size_t colonPos = hostPort.find(':');
            if (colonPos != std::string::npos) {
                opts.host = hostPort.substr(0, colonPos);
                opts.port = std::stoi(hostPort.substr(colonPos + 1));
            }
        }
        
        redis_ = std::make_unique<sw::redis::Redis>(opts);
        redis_->ping(); // Test connection
        
        // Set default handlers
        setJobHandler(JobType::CRAWL_DOMAIN, [this](const Job& job) { return handleCrawlDomain(job); });
        setJobHandler(JobType::SEND_EMAIL, [this](const Job& job) { return handleSendEmail(job); });
        setJobHandler(JobType::BULK_CRAWL, [this](const Job& job) { return handleBulkCrawl(job); });
        
        LOG_INFO("JobQueue initialized with Redis connection");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize JobQueue: " + std::string(e.what()));
        throw;
    }
}

JobQueue::~JobQueue() {
    stopWorkers();
}

std::string JobQueue::addJob(JobType type, const json& data, int maxAttempts) {
    Job job;
    job.id = generateJobId();
    job.type = type;
    job.status = JobStatus::PENDING;
    job.data = data;
    job.attempts = 0;
    job.maxAttempts = maxAttempts;
    job.createdAt = std::chrono::system_clock::now();
    job.scheduledAt = job.createdAt;
    
    try {
        // Store job data
        redis_->set(JOB_DATA_PREFIX + job.id, job.toJson().dump());
        
        // Add to pending queue
        redis_->lpush(QUEUE_PENDING, job.id);
        
        // Update stats
        updateStats(JobStatus::PENDING, JobStatus::PENDING);
        
        LOG_INFO("Added job " + job.id + " to queue");
        return job.id;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to add job to queue: " + std::string(e.what()));
        return "";
    }
}

std::string JobQueue::addDomainCrawlJob(const DomainJob& domainJob) {
    return addJob(JobType::CRAWL_DOMAIN, domainJob.toJson(), 3);
}

std::string JobQueue::addEmailJob(const EmailJob& emailJob) {
    return addJob(JobType::SEND_EMAIL, emailJob.toJson(), 3);
}

std::vector<std::string> JobQueue::addBulkDomainCrawlJobs(const std::vector<DomainJob>& domainJobs) {
    std::vector<std::string> jobIds;
    jobIds.reserve(domainJobs.size());
    
    try {
        // Use Redis pipeline for batch operations
        auto pipe = redis_->pipeline();
        
        for (const auto& domainJob : domainJobs) {
            Job job;
            job.id = generateJobId();
            job.type = JobType::CRAWL_DOMAIN;
            job.status = JobStatus::PENDING;
            job.data = domainJob.toJson();
            job.attempts = 0;
            job.maxAttempts = 3;
            job.createdAt = std::chrono::system_clock::now();
            job.scheduledAt = job.createdAt;
            
            // Store job data
            pipe.set(JOB_DATA_PREFIX + job.id, job.toJson().dump());
            
            // Add to pending queue
            pipe.lpush(QUEUE_PENDING, job.id);
            
            jobIds.push_back(job.id);
        }
        
        // Execute pipeline
        pipe.exec();
        
        // Update stats in batch
        redis_->hincrby(STATS_KEY, "pending", static_cast<long long>(domainJobs.size()));
        redis_->hincrby(STATS_KEY, "total", static_cast<long long>(domainJobs.size()));
        
        LOG_INFO("Added " + std::to_string(domainJobs.size()) + " domain crawl jobs to queue");
        return jobIds;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to add bulk jobs to queue: " + std::string(e.what()));
        return {};
    }
}

void JobQueue::startWorkers(int numWorkers) {
    if (running_) {
        LOG_WARNING("Workers already running");
        return;
    }
    
    running_ = true;
    workers_.reserve(numWorkers);
    
    for (int i = 0; i < numWorkers; ++i) {
        workers_.emplace_back(&JobQueue::workerLoop, this);
        LOG_INFO("Started worker thread " + std::to_string(i));
    }
    
    LOG_INFO("Started " + std::to_string(numWorkers) + " worker threads");
}

void JobQueue::stopWorkers() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers_.clear();
    LOG_INFO("Stopped all worker threads");
}

std::optional<Job> JobQueue::getJob(const std::string& jobId) {
    try {
        auto result = redis_->get(JOB_DATA_PREFIX + jobId);
        if (result) {
            return Job::fromJson(json::parse(*result));
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to get job " + jobId + ": " + std::string(e.what()));
    }
    return std::nullopt;
}

JobQueue::QueueStats JobQueue::getStats() {
    QueueStats stats = {0, 0, 0, 0, 0};
    
    try {
        std::vector<sw::redis::OptionalString> results;
        redis_->hmget(STATS_KEY, {"pending", "processing", "completed", "failed", "total"}, std::back_inserter(results));
        
        if (results.size() >= 5) {
            if (results[0]) stats.pending = std::stoi(*results[0]);
            if (results[1]) stats.processing = std::stoi(*results[1]);
            if (results[2]) stats.completed = std::stoi(*results[2]);
            if (results[3]) stats.failed = std::stoi(*results[3]);
            if (results[4]) stats.total = std::stoi(*results[4]);
        }
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to get queue stats: " + std::string(e.what()));
    }
    
    return stats;
}

void JobQueue::setJobHandler(JobType type, JobHandler handler) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    handlers_[type] = handler;
}

void JobQueue::workerLoop() {
    LOG_INFO("Worker thread started");
    
    while (running_) {
        try {
            auto job = dequeueJob();
            if (job) {
                LOG_INFO("Processing job " + job->id + " (type: " + std::to_string(static_cast<int>(job->type)) + ")");
                
                bool success = processJob(*job);
                
                if (success) {
                    markJobCompleted(*job);
                    LOG_INFO("Job " + job->id + " completed successfully");
                } else {
                    if (job->attempts >= job->maxAttempts) {
                        markJobFailed(*job, "Max attempts reached");
                        LOG_ERROR("Job " + job->id + " failed after " + std::to_string(job->attempts) + " attempts");
                    } else {
                        requeueJob(*job);
                        LOG_WARNING("Job " + job->id + " failed, requeuing (attempt " + std::to_string(job->attempts + 1) + ")");
                    }
                }
            } else {
                // No jobs available, sleep briefly
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Worker loop error: " + std::string(e.what()));
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    
    LOG_INFO("Worker thread stopped");
}

std::optional<Job> JobQueue::dequeueJob() {
    try {
        // Atomic move from pending to processing queue
        auto result = redis_->brpoplpush(QUEUE_PENDING, QUEUE_PROCESSING, std::chrono::seconds(1));
        
        if (result) {
            auto jobData = redis_->get(JOB_DATA_PREFIX + *result);
            if (jobData) {
                auto job = Job::fromJson(json::parse(*jobData));
                job.status = JobStatus::PROCESSING;
                job.attempts++;
                updateJobInRedis(job);
                updateStats(JobStatus::PENDING, JobStatus::PROCESSING);
                return job;
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to dequeue job: " + std::string(e.what()));
    }
    
    return std::nullopt;
}

bool JobQueue::processJob(const Job& job) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    
    auto it = handlers_.find(job.type);
    if (it != handlers_.end()) {
        try {
            return it->second(job);
        } catch (const std::exception& e) {
            LOG_ERROR("Job handler error for job " + job.id + ": " + std::string(e.what()));
            return false;
        }
    }
    
    LOG_ERROR("No handler found for job type: " + std::to_string(static_cast<int>(job.type)));
    return false;
}

void JobQueue::markJobCompleted(const Job& job) {
    try {
        Job completedJob = job;
        completedJob.status = JobStatus::COMPLETED;
        completedJob.completedAt = std::chrono::system_clock::now();
        
        updateJobInRedis(completedJob);
        redis_->lrem(QUEUE_PROCESSING, 1, job.id);
        redis_->lpush(QUEUE_COMPLETED, job.id);
        updateStats(JobStatus::PROCESSING, JobStatus::COMPLETED);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to mark job completed: " + std::string(e.what()));
    }
}

void JobQueue::markJobFailed(const Job& job, const std::string& error) {
    try {
        Job failedJob = job;
        failedJob.status = JobStatus::FAILED;
        failedJob.errorMessage = error;
        failedJob.completedAt = std::chrono::system_clock::now();
        
        updateJobInRedis(failedJob);
        redis_->lrem(QUEUE_PROCESSING, 1, job.id);
        redis_->lpush(QUEUE_FAILED, job.id);
        updateStats(JobStatus::PROCESSING, JobStatus::FAILED);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to mark job failed: " + std::string(e.what()));
    }
}

void JobQueue::requeueJob(const Job& job) {
    try {
        Job requeuedJob = job;
        requeuedJob.status = JobStatus::PENDING;
        requeuedJob.scheduledAt = std::chrono::system_clock::now() + std::chrono::minutes(5); // Delay retry by 5 minutes
        
        updateJobInRedis(requeuedJob);
        redis_->lrem(QUEUE_PROCESSING, 1, job.id);
        redis_->lpush(QUEUE_PENDING, job.id);
        updateStats(JobStatus::PROCESSING, JobStatus::PENDING);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to requeue job: " + std::string(e.what()));
    }
}

std::string JobQueue::generateJobId() {
    // Generate UUID
    uuid_t uuid;
    uuid_generate_random(uuid);
    char uuid_str[37];
    uuid_unparse_lower(uuid, uuid_str);
    return std::string(uuid_str);
}

void JobQueue::updateJobInRedis(const Job& job) {
    redis_->set(JOB_DATA_PREFIX + job.id, job.toJson().dump());
}

void JobQueue::updateStats(JobStatus oldStatus, JobStatus newStatus) {
    try {
        if (oldStatus != newStatus) {
            // Decrement old status count
            if (oldStatus != JobStatus::PENDING) { // Don't decrement if it's a new job
                std::string oldKey;
                switch (oldStatus) {
                    case JobStatus::PENDING: oldKey = "pending"; break;
                    case JobStatus::PROCESSING: oldKey = "processing"; break;
                    case JobStatus::COMPLETED: oldKey = "completed"; break;
                    case JobStatus::FAILED: oldKey = "failed"; break;
                }
                if (!oldKey.empty()) {
                    redis_->hincrby(STATS_KEY, oldKey, -1);
                }
            }
            
            // Increment new status count
            std::string newKey;
            switch (newStatus) {
                case JobStatus::PENDING: newKey = "pending"; break;
                case JobStatus::PROCESSING: newKey = "processing"; break;
                case JobStatus::COMPLETED: newKey = "completed"; break;
                case JobStatus::FAILED: newKey = "failed"; break;
            }
            if (!newKey.empty()) {
                redis_->hincrby(STATS_KEY, newKey, 1);
            }
            
            // Always increment total for new jobs
            if (oldStatus == JobStatus::PENDING && newStatus == JobStatus::PENDING) {
                redis_->hincrby(STATS_KEY, "total", 1);
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to update stats: " + std::string(e.what()));
    }
}

// Default job handlers
bool JobQueue::handleCrawlDomain(const Job& job) {
    try {
        auto domainJob = DomainJob::fromJson(job.data);
        
        LOG_INFO("Starting crawl for domain: " + domainJob.domain);
        
        // Create crawl configuration
        CrawlConfig config;
        config.maxPages = domainJob.maxPages;
        config.maxDepth = 3;
        config.politenessDelay = std::chrono::milliseconds(1000); // 1 second between requests
        config.respectRobotsTxt = true;
        config.restrictToSeedDomain = true;
        
        // Initialize storage if needed
        auto storage = std::make_shared<search_engine::storage::ContentStorage>();
        
        // Create crawler manager
        CrawlerManager crawlerManager(storage);
        
        // Start crawl
        std::string sessionId = crawlerManager.startCrawl("https://" + domainJob.domain, config, false);
        
        if (sessionId.empty()) {
            LOG_ERROR("Failed to start crawl for domain: " + domainJob.domain);
            return false;
        }
        
        // Wait for crawl to complete (with timeout)
        auto startTime = std::chrono::system_clock::now();
        auto timeout = std::chrono::minutes(10); // 10 minute timeout
        
        while (true) {
            auto status = crawlerManager.getCrawlStatus(sessionId);
            if (status == "completed" || status == "failed") {
                break;
            }
            
            // Check timeout
            if (std::chrono::system_clock::now() - startTime > timeout) {
                LOG_ERROR("Crawl timeout for domain: " + domainJob.domain);
                crawlerManager.stopCrawl(sessionId);
                return false;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        
        // Get crawl results
        auto results = crawlerManager.getCrawlResults(sessionId);
        LOG_INFO("Crawl completed for domain: " + domainJob.domain + ", pages crawled: " + std::to_string(results.size()));
        
        // Schedule email job if webmaster email is provided
        if (!domainJob.webmasterEmail.empty()) {
            EmailJob emailJob;
            emailJob.to = domainJob.webmasterEmail;
            emailJob.subject = "Your website has been crawled by our search engine";
            emailJob.templateName = "webmaster_notification";
            emailJob.templateData = json{
                {"domain", domainJob.domain},
                {"pagesCrawled", results.size()},
                {"crawlDate", std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count()}
            };
            emailJob.domain = domainJob.domain;
            
            addEmailJob(emailJob);
        }
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error processing crawl domain job: " + std::string(e.what()));
        return false;
    }
}

bool JobQueue::handleSendEmail(const Job& job) {
    try {
        auto emailJob = EmailJob::fromJson(job.data);
        
        LOG_INFO("Sending email to: " + emailJob.to + " for domain: " + emailJob.domain);
        
        // Here you would integrate with your email service
        // For now, we'll just log the email details
        LOG_INFO("Email details - To: " + emailJob.to + ", Subject: " + emailJob.subject);
        LOG_INFO("Template: " + emailJob.templateName + ", Data: " + emailJob.templateData.dump());
        
        // TODO: Implement actual email sending
        // EmailService emailService;
        // return emailService.sendEmail(emailJob);
        
        // For now, simulate success
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error processing send email job: " + std::string(e.what()));
        return false;
    }
}

bool JobQueue::handleBulkCrawl(const Job& job) {
    // This handler would process bulk crawl operations
    // For now, it's a placeholder
    LOG_INFO("Processing bulk crawl job: " + job.id);
    return true;
}
