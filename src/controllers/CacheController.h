#pragma once

#include "../../include/routing/Controller.h"
#include <atomic>
#include <chrono>

class CacheController : public routing::Controller {
public:
    CacheController();
    
    // Cache statistics endpoint
    void getCacheStats(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // Cache management endpoints
    void clearCache(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    void getCacheInfo(uWS::HttpResponse<false>* res, uWS::HttpRequest* req);
    
    // Static metrics for tracking cache performance
    static std::atomic<uint64_t> totalRequests;
    static std::atomic<uint64_t> cacheHits;
    static std::atomic<uint64_t> cacheMisses;
    static std::atomic<uint64_t> minificationTime;  // Total time in microseconds
    static std::atomic<uint64_t> cacheTime;         // Total time in microseconds
    
    // Helper methods
    static void recordCacheHit(uint64_t cacheTimeUs);
    static void recordCacheMiss(uint64_t minificationTimeUs);
    static double getCacheHitRate();
    static double getAverageMinificationTime();
    static double getAverageCacheTime();
};
