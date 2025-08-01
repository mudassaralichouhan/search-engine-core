#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include "models/CrawlConfig.h"
#include "FailureClassifier.h"

enum class CircuitBreakerState {
    CLOSED,     // Normal operation
    OPEN,       // Circuit breaker triggered, blocking requests
    HALF_OPEN   // Testing if domain has recovered
};

struct DomainState {
    // Circuit breaker state
    CircuitBreakerState circuitState = CircuitBreakerState::CLOSED;
    
    // Failure tracking
    int consecutiveFailures = 0;
    int totalRequests = 0;
    int successfulRequests = 0;
    
    // Timing information
    std::chrono::system_clock::time_point lastRequest = std::chrono::system_clock::time_point::min();
    std::chrono::system_clock::time_point circuitOpenedAt = std::chrono::system_clock::time_point::min();
    std::chrono::system_clock::time_point lastSuccessfulRequest = std::chrono::system_clock::time_point::min();
    
    // Dynamic crawl delay (starts at config value, increases with failures)
    std::chrono::milliseconds dynamicCrawlDelay{0};
    
    // Rate limiting
    bool isRateLimited = false;
    std::chrono::system_clock::time_point rateLimitResetTime = std::chrono::system_clock::time_point::min();
    
    // Error tracking
    std::string lastError;
    FailureType lastFailureType = FailureType::UNKNOWN;
    
    // Calculate success rate
    double getSuccessRate() const {
        return totalRequests > 0 ? static_cast<double>(successfulRequests) / totalRequests : 0.0;
    }
    
    // Check if domain should be crawled now
    bool canCrawlNow(const std::chrono::system_clock::time_point& now) const {
        return now >= (lastRequest + dynamicCrawlDelay);
    }
};

class DomainManager {
public:
    explicit DomainManager(const CrawlConfig& config);
    ~DomainManager() = default;
    
    /**
     * Check if a domain should be delayed before crawling
     * @param domain The domain to check
     * @return true if should delay, false if can proceed
     */
    bool shouldDelay(const std::string& domain);
    
    /**
     * Get the recommended delay before crawling this domain
     * @param domain The domain to check
     * @return Delay in milliseconds
     */
    std::chrono::milliseconds getDelay(const std::string& domain);
    
    /**
     * Check if circuit breaker is open for this domain
     * @param domain The domain to check
     * @return true if circuit breaker is open (should not crawl)
     */
    bool isCircuitBreakerOpen(const std::string& domain);
    
    /**
     * Record a successful request for a domain
     * @param domain The domain
     */
    void recordSuccess(const std::string& domain);
    
    /**
     * Record a failed request for a domain
     * @param domain The domain
     * @param failureType Type of failure
     * @param error Error message
     */
    void recordFailure(const std::string& domain, FailureType failureType, const std::string& error);
    
    /**
     * Record a rate-limited response (429) for a domain
     * @param domain The domain
     * @param retryAfter Optional Retry-After header value in seconds
     */
    void recordRateLimit(const std::string& domain, int retryAfter = 0);
    
    /**
     * Get statistics for a domain
     * @param domain The domain
     * @return DomainState with current statistics
     */
    DomainState getDomainState(const std::string& domain) const;
    
    /**
     * Get statistics for all domains
     * @return Map of domain names to their states
     */
    std::unordered_map<std::string, DomainState> getAllDomainStates() const;
    
    /**
     * Reset circuit breaker for a domain (manual override)
     * @param domain The domain
     */
    void resetCircuitBreaker(const std::string& domain);
    
    /**
     * Update configuration
     * @param newConfig New configuration
     */
    void updateConfig(const CrawlConfig& newConfig);

private:
    /**
     * Update circuit breaker state for a domain
     * @param domain The domain
     * @param state Reference to domain state
     */
    void updateCircuitBreakerState(const std::string& domain, DomainState& state);
    
    /**
     * Calculate dynamic crawl delay based on failure rate
     * @param state Domain state
     * @return New delay in milliseconds
     */
    std::chrono::milliseconds calculateDynamicDelay(const DomainState& state) const;
    
    /**
     * Get or create domain state
     * @param domain The domain
     * @return Reference to domain state
     */
    DomainState& getOrCreateDomainState(const std::string& domain);
    
    // Configuration
    CrawlConfig config_;
    
    // Domain states
    std::unordered_map<std::string, DomainState> domainStates_;
    
    // Thread safety
    mutable std::mutex domainMutex_;
};