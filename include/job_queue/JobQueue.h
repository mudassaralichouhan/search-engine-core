#pragma once

#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <nlohmann/json.hpp>
#include <sw/redis++/redis++.h>
#include "../Logger.h"

using json = nlohmann::json;

enum class JobType {
    CRAWL_DOMAIN = 1,
    SEND_EMAIL = 2,
    BULK_CRAWL = 3
};

enum class JobStatus {
    PENDING = 0,
    PROCESSING = 1,
    COMPLETED = 2,
    FAILED = 3,
    RETRYING = 4
};

struct Job {
    std::string id;
    JobType type;
    JobStatus status;
    json data;
    int attempts;
    int maxAttempts;
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point scheduledAt;
    std::chrono::system_clock::time_point completedAt;
    std::string errorMessage;
    
    // Serialization
    json toJson() const;
    static Job fromJson(const json& j);
};

struct DomainJob {
    std::string domain;
    std::string webmasterEmail;
    int maxPages;
    std::string sessionId;
    std::chrono::system_clock::time_point createdAt;
    
    json toJson() const;
    static DomainJob fromJson(const json& j);
};

struct EmailJob {
    std::string to;
    std::string subject;
    std::string templateName;
    json templateData;
    std::string domain;
    
    json toJson() const;
    static EmailJob fromJson(const json& j);
};

class JobQueue {
public:
    JobQueue(const std::string& redisUri = "tcp://redis:6379");
    ~JobQueue();
    
    // Job management
    std::string addJob(JobType type, const json& data, int maxAttempts = 3);
    std::string addDomainCrawlJob(const DomainJob& domainJob);
    std::string addEmailJob(const EmailJob& emailJob);
    
    // Bulk operations
    std::vector<std::string> addBulkDomainCrawlJobs(const std::vector<DomainJob>& domainJobs);
    
    // Job processing
    void startWorkers(int numWorkers = 4);
    void stopWorkers();
    bool isRunning() const { return running_; }
    
    // Job status
    std::optional<Job> getJob(const std::string& jobId);
    std::vector<Job> getJobsByStatus(JobStatus status, int limit = 100);
    std::vector<Job> getJobsByType(JobType type, int limit = 100);
    
    // Statistics
    struct QueueStats {
        int pending;
        int processing;
        int completed;
        int failed;
        int total;
    };
    QueueStats getStats();
    
    // Job handlers
    using JobHandler = std::function<bool(const Job&)>;
    void setJobHandler(JobType type, JobHandler handler);
    
private:
    std::unique_ptr<sw::redis::Redis> redis_;
    std::atomic<bool> running_{false};
    std::vector<std::thread> workers_;
    std::mutex handlersMutex_;
    std::unordered_map<JobType, JobHandler> handlers_;
    
    // Redis keys
    static const std::string QUEUE_PENDING;
    static const std::string QUEUE_PROCESSING;
    static const std::string QUEUE_COMPLETED;
    static const std::string QUEUE_FAILED;
    static const std::string JOB_DATA_PREFIX;
    static const std::string STATS_KEY;
    
    // Worker methods
    void workerLoop();
    std::optional<Job> dequeueJob();
    bool processJob(const Job& job);
    void markJobCompleted(const Job& job);
    void markJobFailed(const Job& job, const std::string& error);
    void requeueJob(const Job& job);
    
    // Utility methods
    std::string generateJobId();
    void updateJobInRedis(const Job& job);
    void updateStats(JobStatus oldStatus, JobStatus newStatus);
    
    // Default handlers
    bool handleCrawlDomain(const Job& job);
    bool handleSendEmail(const Job& job);
    bool handleBulkCrawl(const Job& job);
};
