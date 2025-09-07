#pragma once

#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include "../../include/job_queue/JobQueue.h"
#include "../../include/storage/DomainStorage.h"
#include "../storage/EmailService.h"

/**
 * Background service that manages job queue workers and handles system-wide
 * crawling and email operations.
 */
class JobWorkerService {
public:
    JobWorkerService(int workerCount = 4);
    ~JobWorkerService();
    
    // Service lifecycle
    bool start();
    void stop();
    bool isRunning() const { return running_; }
    
    // Configuration
    void setWorkerCount(int count);
    void setEmailConfig(const EmailConfig& config);
    
    // Statistics
    struct ServiceStats {
        bool running;
        int workerCount;
        int activeJobs;
        int completedJobs;
        int failedJobs;
        std::chrono::system_clock::time_point startedAt;
        std::chrono::seconds uptime;
    };
    ServiceStats getStats() const;
    
    // Health check
    bool healthCheck() const;
    
    // Access to components
    JobQueue* getJobQueue() const { return jobQueue_.get(); }
    DomainStorage* getDomainStorage() const { return domainStorage_.get(); }
    EmailService* getEmailService() const { return emailService_.get(); }
    
private:
    std::unique_ptr<JobQueue> jobQueue_;
    std::unique_ptr<DomainStorage> domainStorage_;
    std::unique_ptr<EmailService> emailService_;
    
    std::atomic<bool> running_{false};
    int workerCount_;
    std::chrono::system_clock::time_point startedAt_;
    
    // Monitoring thread
    std::thread monitoringThread_;
    std::atomic<bool> shouldStop_{false};
    
    void monitoringLoop();
    void setupCustomJobHandlers();
    bool handleCrawlDomainJob(const Job& job);
    bool handleEmailJob(const Job& job);
    
    // Initialize services
    bool initializeServices();
};
