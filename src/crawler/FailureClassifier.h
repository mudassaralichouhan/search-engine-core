#pragma once

#include <string>
#include <curl/curl.h>
#include "models/CrawlConfig.h"
#include "../../include/search_engine/crawler/models/FailureType.h"

class FailureClassifier {
public:
    /**
     * Classify a failure based on HTTP status code, CURL error, and error message
     * @param httpCode HTTP status code (0 if not HTTP-related)
     * @param curlCode CURL error code
     * @param errorMessage Error message string
     * @param config Crawl configuration with retry settings
     * @return FailureType indicating how to handle this failure
     */
    static FailureType classifyFailure(int httpCode, 
                                     CURLcode curlCode, 
                                     const std::string& errorMessage,
                                     const CrawlConfig& config);
    
    /**
     * Check if a failure should be retried based on its classification
     * @param failureType The type of failure
     * @param retryCount Current number of retries attempted
     * @param maxRetries Maximum allowed retries
     * @return true if should retry, false otherwise
     */
    static bool shouldRetry(FailureType failureType, int retryCount, int maxRetries);
    
    /**
     * Calculate the delay before next retry attempt using exponential backoff
     * @param retryCount Current retry attempt number (1-based)
     * @param config Crawl configuration with retry settings
     * @param failureType Type of failure (affects delay calculation)
     * @return Delay in milliseconds before next retry
     */
    static std::chrono::milliseconds calculateRetryDelay(int retryCount, 
                                                       const CrawlConfig& config,
                                                       FailureType failureType);
    
    /**
     * Get a human-readable description of the failure type
     * @param failureType The failure type
     * @return String description
     */
    static std::string getFailureTypeDescription(FailureType failureType);

private:
    /**
     * Check if HTTP status code indicates a permanent failure
     * @param httpCode HTTP status code
     * @return true if permanent failure
     */
    static bool isPermanentHttpError(int httpCode);
    
    /**
     * Check if CURL error indicates a permanent failure
     * @param curlCode CURL error code
     * @return true if permanent failure
     */
    static bool isPermanentCurlError(CURLcode curlCode);
};