#pragma once

#include <atomic>
#include <unordered_map>
#include <string>
#include <mutex>
#include <chrono>
#include "FailureClassifier.h"

// Internal structure with atomic members (used within CrawlMetrics)
struct DomainMetrics {
    std::atomic<size_t> totalRequests{0};
    std::atomic<size_t> successfulRequests{0};
    std::atomic<size_t> failedRequests{0};
    std::atomic<size_t> retriedRequests{0};
    std::atomic<size_t> circuitBreakerTriggered{0};
    std::atomic<size_t> rateLimitedRequests{0};
    
    double getSuccessRate() const {
        size_t total = totalRequests.load();
        return total > 0 ? static_cast<double>(successfulRequests.load()) / total : 0.0;
    }
    
    double getRetryRate() const {
        size_t total = totalRequests.load();
        return total > 0 ? static_cast<double>(retriedRequests.load()) / total : 0.0;
    }
};

// Public structure with regular members (returned by getter functions)
struct DomainMetricsSnapshot {
    size_t totalRequests = 0;
    size_t successfulRequests = 0;
    size_t failedRequests = 0;
    size_t retriedRequests = 0;
    size_t circuitBreakerTriggered = 0;
    size_t rateLimitedRequests = 0;
    
    double getSuccessRate() const {
        return totalRequests > 0 ? static_cast<double>(successfulRequests) / totalRequests : 0.0;
    }
    
    double getRetryRate() const {
        return totalRequests > 0 ? static_cast<double>(retriedRequests) / totalRequests : 0.0;
    }
};

class CrawlMetrics {
public:
    CrawlMetrics() = default;
    ~CrawlMetrics() = default;
    
    // Global metrics
    void recordRequest() { totalRequests_.fetch_add(1); }
    void recordSuccess() { successfulRequests_.fetch_add(1); }
    void recordFailure() { failedRequests_.fetch_add(1); }
    void recordRetry() { retriedRequests_.fetch_add(1); }
    void recordPermanentFailure() { permanentFailures_.fetch_add(1); }
    void recordCircuitBreakerTriggered() { circuitBreakerTriggered_.fetch_add(1); }
    void recordRateLimit() { rateLimitedRequests_.fetch_add(1); }
    
    // Domain-specific metrics
    void recordDomainRequest(const std::string& domain) {
        std::lock_guard<std::mutex> lock(domainMutex_);
        domainMetrics_[domain].totalRequests.fetch_add(1);
    }
    
    void recordDomainSuccess(const std::string& domain) {
        std::lock_guard<std::mutex> lock(domainMutex_);
        domainMetrics_[domain].successfulRequests.fetch_add(1);
    }
    
    void recordDomainFailure(const std::string& domain) {
        std::lock_guard<std::mutex> lock(domainMutex_);
        domainMetrics_[domain].failedRequests.fetch_add(1);
    }
    
    void recordDomainRetry(const std::string& domain) {
        std::lock_guard<std::mutex> lock(domainMutex_);
        domainMetrics_[domain].retriedRequests.fetch_add(1);
    }
    
    void recordDomainCircuitBreaker(const std::string& domain) {
        std::lock_guard<std::mutex> lock(domainMutex_);
        domainMetrics_[domain].circuitBreakerTriggered.fetch_add(1);
    }
    
    void recordDomainRateLimit(const std::string& domain) {
        std::lock_guard<std::mutex> lock(domainMutex_);
        domainMetrics_[domain].rateLimitedRequests.fetch_add(1);
    }
    
    // Failure type tracking
    void recordFailureType(FailureType type) {
        std::lock_guard<std::mutex> lock(failureTypeMutex_);
        failureTypeCounts_[type]++;
    }
    
    // Getters for global metrics
    size_t getTotalRequests() const { return totalRequests_.load(); }
    size_t getSuccessfulRequests() const { return successfulRequests_.load(); }
    size_t getFailedRequests() const { return failedRequests_.load(); }
    size_t getRetriedRequests() const { return retriedRequests_.load(); }
    size_t getPermanentFailures() const { return permanentFailures_.load(); }
    size_t getCircuitBreakerTriggered() const { return circuitBreakerTriggered_.load(); }
    size_t getRateLimitedRequests() const { return rateLimitedRequests_.load(); }
    
    double getSuccessRate() const {
        size_t total = totalRequests_.load();
        return total > 0 ? static_cast<double>(successfulRequests_.load()) / total : 0.0;
    }
    
    double getRetryRate() const {
        size_t total = totalRequests_.load();
        return total > 0 ? static_cast<double>(retriedRequests_.load()) / total : 0.0;
    }
    
    // Domain metrics getter (returns non-atomic snapshot)
    DomainMetricsSnapshot getDomainMetrics(const std::string& domain) const {
        std::lock_guard<std::mutex> lock(domainMutex_);
        auto it = domainMetrics_.find(domain);
        if (it != domainMetrics_.end()) {
            DomainMetricsSnapshot snapshot;
            snapshot.totalRequests = it->second.totalRequests.load();
            snapshot.successfulRequests = it->second.successfulRequests.load();
            snapshot.failedRequests = it->second.failedRequests.load();
            snapshot.retriedRequests = it->second.retriedRequests.load();
            snapshot.circuitBreakerTriggered = it->second.circuitBreakerTriggered.load();
            snapshot.rateLimitedRequests = it->second.rateLimitedRequests.load();
            return snapshot;
        }
        return DomainMetricsSnapshot{};
    }
    
    // Get all domain metrics (returns non-atomic snapshots)
    std::unordered_map<std::string, DomainMetricsSnapshot> getAllDomainMetrics() const {
        std::lock_guard<std::mutex> lock(domainMutex_);
        std::unordered_map<std::string, DomainMetricsSnapshot> result;
        for (const auto& [domain, metrics] : domainMetrics_) {
            DomainMetricsSnapshot snapshot;
            snapshot.totalRequests = metrics.totalRequests.load();
            snapshot.successfulRequests = metrics.successfulRequests.load();
            snapshot.failedRequests = metrics.failedRequests.load();
            snapshot.retriedRequests = metrics.retriedRequests.load();
            snapshot.circuitBreakerTriggered = metrics.circuitBreakerTriggered.load();
            snapshot.rateLimitedRequests = metrics.rateLimitedRequests.load();
            result[domain] = snapshot;
        }
        return result;
    }
    
    // Get failure type counts
    std::unordered_map<FailureType, size_t> getFailureTypeCounts() const {
        std::lock_guard<std::mutex> lock(failureTypeMutex_);
        return failureTypeCounts_;
    }
    
    // Reset all metrics
    void reset() {
        totalRequests_.store(0);
        successfulRequests_.store(0);
        failedRequests_.store(0);
        retriedRequests_.store(0);
        permanentFailures_.store(0);
        circuitBreakerTriggered_.store(0);
        rateLimitedRequests_.store(0);
        
        {
            std::lock_guard<std::mutex> lock(domainMutex_);
            domainMetrics_.clear();
        }
        
        {
            std::lock_guard<std::mutex> lock(failureTypeMutex_);
            failureTypeCounts_.clear();
        }
    }
    
    // Log metrics summary
    void logSummary() const;

private:
    // Global metrics
    std::atomic<size_t> totalRequests_{0};
    std::atomic<size_t> successfulRequests_{0};
    std::atomic<size_t> failedRequests_{0};
    std::atomic<size_t> retriedRequests_{0};
    std::atomic<size_t> permanentFailures_{0};
    std::atomic<size_t> circuitBreakerTriggered_{0};
    std::atomic<size_t> rateLimitedRequests_{0};
    
    // Domain-specific metrics
    mutable std::mutex domainMutex_;
    std::unordered_map<std::string, DomainMetrics> domainMetrics_;
    
    // Failure type tracking
    mutable std::mutex failureTypeMutex_;
    std::unordered_map<FailureType, size_t> failureTypeCounts_;
};