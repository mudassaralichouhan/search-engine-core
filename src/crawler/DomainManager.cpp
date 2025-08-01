#include "DomainManager.h"
#include "../../include/Logger.h"
#include "../../include/crawler/CrawlLogger.h"
#include <algorithm>
#include <cmath>

DomainManager::DomainManager(const CrawlConfig& config) : config_(config) {
    LOG_DEBUG("DomainManager initialized with circuit breaker threshold: " + 
              std::to_string(config_.circuitBreakerFailureThreshold));
}

bool DomainManager::shouldDelay(const std::string& domain) {
    std::lock_guard<std::mutex> lock(domainMutex_);
    
    auto& state = getOrCreateDomainState(domain);
    auto now = std::chrono::system_clock::now();
    
    // Check circuit breaker
    updateCircuitBreakerState(domain, state);
    if (state.circuitState == CircuitBreakerState::OPEN) {
        LOG_DEBUG("Circuit breaker OPEN for domain: " + domain + ", blocking request");
        return true;
    }
    
    // Check rate limiting
    if (state.isRateLimited && now < state.rateLimitResetTime) {
        LOG_DEBUG("Domain is rate limited: " + domain + ", blocking until reset");
        return true;
    } else if (state.isRateLimited && now >= state.rateLimitResetTime) {
        // Rate limit has expired
        state.isRateLimited = false;
        LOG_INFO("Rate limit expired for domain: " + domain);
    }
    
    // Check crawl delay
    if (!state.canCrawlNow(now)) {
        auto remainingDelay = std::chrono::duration_cast<std::chrono::milliseconds>(
            (state.lastRequest + state.dynamicCrawlDelay) - now
        );
        LOG_DEBUG("Domain " + domain + " requires delay of " + 
                 std::to_string(remainingDelay.count()) + "ms");
        return true;
    }
    
    return false;
}

std::chrono::milliseconds DomainManager::getDelay(const std::string& domain) {
    std::lock_guard<std::mutex> lock(domainMutex_);
    
    auto& state = getOrCreateDomainState(domain);
    auto now = std::chrono::system_clock::now();
    
    // Check circuit breaker delay
    if (state.circuitState == CircuitBreakerState::OPEN) {
        auto timeSinceOpened = now - state.circuitOpenedAt;
        auto resetTime = config_.circuitBreakerResetTime;
        if (timeSinceOpened < resetTime) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(resetTime - timeSinceOpened);
        }
    }
    
    // Check rate limit delay
    if (state.isRateLimited && now < state.rateLimitResetTime) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(state.rateLimitResetTime - now);
    }
    
    // Check crawl delay
    if (!state.canCrawlNow(now)) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            (state.lastRequest + state.dynamicCrawlDelay) - now
        );
    }
    
    return std::chrono::milliseconds(0);
}

bool DomainManager::isCircuitBreakerOpen(const std::string& domain) {
    std::lock_guard<std::mutex> lock(domainMutex_);
    
    auto& state = getOrCreateDomainState(domain);
    updateCircuitBreakerState(domain, state);
    
    return state.circuitState == CircuitBreakerState::OPEN;
}

void DomainManager::recordSuccess(const std::string& domain) {
    std::lock_guard<std::mutex> lock(domainMutex_);
    
    auto& state = getOrCreateDomainState(domain);
    auto now = std::chrono::system_clock::now();
    
    state.totalRequests++;
    state.successfulRequests++;
    state.consecutiveFailures = 0;  // Reset failure count
    state.lastRequest = now;
    state.lastSuccessfulRequest = now;
    
    // Update circuit breaker state (might transition to CLOSED)
    updateCircuitBreakerState(domain, state);
    
    // Reduce dynamic delay on success
    if (state.dynamicCrawlDelay > config_.politenessDelay) {
        state.dynamicCrawlDelay = std::max(
            config_.politenessDelay,
            std::chrono::milliseconds(static_cast<long>(state.dynamicCrawlDelay.count() * 0.8))
        );
    }
    
    LOG_DEBUG("Recorded success for domain: " + domain + 
              " (Success rate: " + std::to_string(state.getSuccessRate() * 100) + "%, " +
              "Dynamic delay: " + std::to_string(state.dynamicCrawlDelay.count()) + "ms)");
}

void DomainManager::recordFailure(const std::string& domain, FailureType failureType, const std::string& error) {
    std::lock_guard<std::mutex> lock(domainMutex_);
    
    auto& state = getOrCreateDomainState(domain);
    auto now = std::chrono::system_clock::now();
    
    state.totalRequests++;
    state.consecutiveFailures++;
    state.lastRequest = now;
    state.lastError = error;
    state.lastFailureType = failureType;
    
    // Update circuit breaker state (might open the circuit)
    updateCircuitBreakerState(domain, state);
    
    // Increase dynamic delay based on failure type
    auto newDelay = calculateDynamicDelay(state);
    state.dynamicCrawlDelay = newDelay;
    
    LOG_WARNING("Recorded failure for domain: " + domain + 
                " (Consecutive failures: " + std::to_string(state.consecutiveFailures) + 
                ", Success rate: " + std::to_string(state.getSuccessRate() * 100) + "%, " +
                "Dynamic delay: " + std::to_string(state.dynamicCrawlDelay.count()) + "ms, " +
                "Circuit: " + (state.circuitState == CircuitBreakerState::OPEN ? "OPEN" : 
                             state.circuitState == CircuitBreakerState::HALF_OPEN ? "HALF_OPEN" : "CLOSED") + ")");
    
    if (state.circuitState == CircuitBreakerState::OPEN) {
        CrawlLogger::broadcastLog("🚨 Circuit breaker OPENED for domain: " + domain + 
                                " after " + std::to_string(state.consecutiveFailures) + " failures", "warning");
    }
}

void DomainManager::recordRateLimit(const std::string& domain, int retryAfter) {
    std::lock_guard<std::mutex> lock(domainMutex_);
    
    auto& state = getOrCreateDomainState(domain);
    auto now = std::chrono::system_clock::now();
    
    state.isRateLimited = true;
    
    // Use provided Retry-After or default rate limit delay
    auto rateLimitDuration = retryAfter > 0 ? 
        std::chrono::seconds(retryAfter) : 
        config_.rateLimitDelay;
    
    state.rateLimitResetTime = now + rateLimitDuration;
    
    // Also increase dynamic delay
    state.dynamicCrawlDelay = std::max(
        state.dynamicCrawlDelay,
        std::chrono::duration_cast<std::chrono::milliseconds>(rateLimitDuration)
    );
    
    LOG_WARNING("Rate limit recorded for domain: " + domain + 
                ", reset in " + std::to_string(rateLimitDuration.count()) + " seconds");
    CrawlLogger::broadcastLog("⏰ Rate limited by domain: " + domain + 
                            " for " + std::to_string(rateLimitDuration.count()) + " seconds", "warning");
}

DomainState DomainManager::getDomainState(const std::string& domain) const {
    std::lock_guard<std::mutex> lock(domainMutex_);
    
    auto it = domainStates_.find(domain);
    if (it != domainStates_.end()) {
        return it->second;
    }
    
    // Return default state
    DomainState defaultState;
    defaultState.dynamicCrawlDelay = config_.politenessDelay;
    return defaultState;
}

std::unordered_map<std::string, DomainState> DomainManager::getAllDomainStates() const {
    std::lock_guard<std::mutex> lock(domainMutex_);
    return domainStates_;
}

void DomainManager::resetCircuitBreaker(const std::string& domain) {
    std::lock_guard<std::mutex> lock(domainMutex_);
    
    auto& state = getOrCreateDomainState(domain);
    state.circuitState = CircuitBreakerState::CLOSED;
    state.consecutiveFailures = 0;
    state.circuitOpenedAt = std::chrono::system_clock::time_point::min();
    state.dynamicCrawlDelay = config_.politenessDelay;
    
    LOG_INFO("Circuit breaker manually reset for domain: " + domain);
    CrawlLogger::broadcastLog("🔧 Circuit breaker reset for domain: " + domain, "info");
}

void DomainManager::updateConfig(const CrawlConfig& newConfig) {
    std::lock_guard<std::mutex> lock(domainMutex_);
    config_ = newConfig;
    LOG_INFO("DomainManager configuration updated");
}

void DomainManager::updateCircuitBreakerState(const std::string& domain, DomainState& state) {
    auto now = std::chrono::system_clock::now();
    
    switch (state.circuitState) {
        case CircuitBreakerState::CLOSED:
            // Check if we should open the circuit
            if (state.consecutiveFailures >= config_.circuitBreakerFailureThreshold) {
                state.circuitState = CircuitBreakerState::OPEN;
                state.circuitOpenedAt = now;
                LOG_WARNING("Circuit breaker OPENED for domain: " + domain);
            }
            break;
            
        case CircuitBreakerState::OPEN:
            // Check if we should transition to half-open
            if (now >= (state.circuitOpenedAt + config_.circuitBreakerResetTime)) {
                state.circuitState = CircuitBreakerState::HALF_OPEN;
                LOG_INFO("Circuit breaker transitioned to HALF_OPEN for domain: " + domain);
            }
            break;
            
        case CircuitBreakerState::HALF_OPEN:
            // This state is handled by recordSuccess/recordFailure
            // Success will close the circuit, failure will open it again
            break;
    }
}

std::chrono::milliseconds DomainManager::calculateDynamicDelay(const DomainState& state) const {
    auto baseDelay = config_.politenessDelay;
    
    // Exponential backoff based on consecutive failures
    double multiplier = std::pow(1.5, std::min(state.consecutiveFailures, 10)); // Cap at 10 for sanity
    
    // Consider failure type
    if (state.lastFailureType == FailureType::RATE_LIMITED) {
        multiplier *= 2.0; // Extra penalty for rate limiting
    } else if (state.lastFailureType == FailureType::TEMPORARY) {
        multiplier *= 1.5; // Moderate penalty for temporary failures
    }
    
    auto newDelay = std::chrono::milliseconds(
        static_cast<long>(baseDelay.count() * multiplier)
    );
    
    // Cap the delay at 5 minutes
    auto maxDelay = std::chrono::minutes(5);
    return std::min(newDelay, std::chrono::duration_cast<std::chrono::milliseconds>(maxDelay));
}

DomainState& DomainManager::getOrCreateDomainState(const std::string& domain) {
    auto it = domainStates_.find(domain);
    if (it == domainStates_.end()) {
        // Create new domain state
        DomainState newState;
        newState.dynamicCrawlDelay = config_.politenessDelay;
        domainStates_[domain] = newState;
        LOG_DEBUG("Created new domain state for: " + domain);
        return domainStates_[domain];
    }
    return it->second;
}