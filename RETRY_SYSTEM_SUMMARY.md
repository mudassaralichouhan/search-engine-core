# Web Crawler Retry System Implementation

## Overview

I've implemented a comprehensive retry mechanism for the web crawler that addresses the original problem of failed/timeout URLs never being retried. The system includes intelligent failure classification, exponential backoff, circuit breakers, and detailed metrics.

## Key Components

### 1. **Enhanced CrawlConfig** (`src/crawler/models/CrawlConfig.h`)
- Added retry configuration with exponential backoff settings
- Configurable HTTP and CURL error codes that trigger retries
- Circuit breaker configuration
- Rate limiting settings

**Key Settings:**
- `maxRetries = 3` - Maximum retry attempts per URL
- `baseRetryDelay = 1000ms` - Base delay for first retry
- `backoffMultiplier = 2.0f` - Exponential backoff multiplier
- `maxRetryDelay = 30000ms` - Maximum retry delay cap
- `circuitBreakerFailureThreshold = 5` - Failures before circuit breaker opens
- `circuitBreakerResetTime = 5 minutes` - Time before trying to close circuit

### 2. **Failure Classification System** (`src/crawler/FailureClassifier.h/cpp`)
Intelligently classifies failures into categories:
- **TEMPORARY**: Network timeouts, server errors (5xx) - retry with exponential backoff
- **RATE_LIMITED**: 429 responses - retry with longer delays
- **PERMANENT**: 404, 403, DNS failures - don't retry
- **UNKNOWN**: Unrecognized errors - retry with caution

### 3. **Enhanced URL Frontier** (`src/crawler/URLFrontier.h/cpp`)
- **Priority Queue System**: Separate main and retry queues
- **Scheduled Retries**: URLs scheduled for retry with specific timing
- **Priority Handling**: Retries get higher priority than new URLs
- **Time-based Processing**: Only processes URLs when they're ready

**Key Features:**
- `scheduleRetry()` - Schedule URL for retry with delay
- `hasReadyURLs()` - Check if any URLs are ready for processing
- `getRetryStats()` - Get retry queue statistics

### 4. **Domain Circuit Breaker** (`src/crawler/DomainManager.h/cpp`)
Prevents hammering failing domains:
- **Circuit States**: CLOSED (normal) → OPEN (blocking) → HALF_OPEN (testing)
- **Dynamic Delays**: Increases crawl delay based on failure rate
- **Rate Limit Handling**: Respects 429 responses and Retry-After headers
- **Success Recovery**: Gradually reduces delays on successful requests

**Circuit Breaker Logic:**
- Opens after 5 consecutive failures
- Stays open for 5 minutes
- Transitions to half-open for testing
- Closes on successful request in half-open state

### 5. **Comprehensive Metrics** (`src/crawler/CrawlMetrics.h/cpp`)
Tracks detailed statistics:
- **Global Metrics**: Total requests, success rate, retry rate
- **Domain Metrics**: Per-domain statistics and health
- **Failure Types**: Breakdown by failure classification
- **Circuit Breaker Events**: Tracks when domains are blocked

### 6. **Enhanced Crawler Logic** (`src/crawler/Crawler.cpp`)
**Intelligent Retry Flow:**
1. Check domain circuit breaker status
2. Check domain delays and rate limits
3. Process URL with failure classification
4. Schedule retries with exponential backoff
5. Record comprehensive metrics
6. Mark permanent failures to prevent infinite loops

## Retry Flow Example

```
URL fails with timeout (TEMPORARY failure, attempt 1/3)
  ↓
Classify as TEMPORARY → Schedule retry in 1 second
  ↓
Retry fails again (attempt 2/3) → Schedule retry in 2 seconds  
  ↓
Retry succeeds → Mark as completed, reset domain delays
```

## Circuit Breaker Example

```
Domain has 5 consecutive failures
  ↓
Circuit breaker OPENS → Block all requests to domain for 5 minutes
  ↓
After 5 minutes → Transition to HALF_OPEN
  ↓
Next request succeeds → Circuit CLOSES, resume normal operation
Next request fails → Circuit OPENS again
```

## Configuration Examples

### Conservative Settings (Production)
```cpp
config.maxRetries = 2;
config.baseRetryDelay = std::chrono::seconds(2);
config.backoffMultiplier = 2.0f;
config.circuitBreakerFailureThreshold = 3;
```

### Aggressive Settings (Testing)
```cpp
config.maxRetries = 5;
config.baseRetryDelay = std::chrono::milliseconds(500);
config.backoffMultiplier = 1.5f;
config.circuitBreakerFailureThreshold = 10;
```

## Benefits

### 1. **Reliability**
- Automatically retries temporary failures (network issues, server overload)
- Distinguishes between temporary and permanent failures
- Handles rate limiting gracefully

### 2. **Efficiency**
- Prevents wasting resources on permanent failures (404, etc.)
- Uses exponential backoff to avoid overwhelming servers
- Circuit breakers prevent hammering failing domains

### 3. **Observability**
- Comprehensive metrics for debugging and optimization
- Real-time WebSocket updates on retry status
- Detailed logging with failure classifications

### 4. **Respectful Crawling**
- Honors robots.txt and crawl delays
- Respects rate limiting (429 responses)
- Implements politeness delays between requests

## Real-World Impact

**Before:** URLs that failed due to temporary issues (timeouts, server overload) were permanently marked as failed and never retried.

**After:** 
- Temporary network issues are automatically retried with exponential backoff
- Permanent failures (404, DNS errors) are quickly identified and not retried
- Rate-limited domains get appropriate delays
- Circuit breakers prevent excessive load on failing domains
- Comprehensive metrics provide insights into crawl health

## Monitoring & Debugging

The system provides extensive logging and metrics:

```
=== CRAWL METRICS SUMMARY ===
Total Requests: 1250
Successful: 1100 (88.0%)
Failed: 150 (12.0%)
Retried: 45 (3.6%)
Permanent Failures: 105
Circuit Breakers Triggered: 3
Rate Limited: 8
```

This retry system transforms the crawler from a "one-shot" system into a robust, intelligent crawler that maximizes successful page retrieval while being respectful to target servers.