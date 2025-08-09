#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include "Crawler.h"
#include "models/CrawlConfig.h"
#include "models/CrawlResult.h"
#include "../../include/search_engine/storage/ContentStorage.h"

struct CrawlSession {
    std::string id;
    std::unique_ptr<Crawler> crawler;
    std::chrono::system_clock::time_point createdAt;
    std::atomic<bool> isCompleted{false};
    std::thread crawlThread;
    
    CrawlSession(const std::string& sessionId, std::unique_ptr<Crawler> crawlerInstance)
        : id(sessionId), crawler(std::move(crawlerInstance)), createdAt(std::chrono::system_clock::now()) {}
    
    // Move constructor
    CrawlSession(CrawlSession&& other) noexcept
        : id(std::move(other.id))
        , crawler(std::move(other.crawler))
        , createdAt(other.createdAt)
        , isCompleted(other.isCompleted.load())
        , crawlThread(std::move(other.crawlThread)) {}
    
    // Disable copy constructor and assignment
    CrawlSession(const CrawlSession&) = delete;
    CrawlSession& operator=(const CrawlSession&) = delete;
    CrawlSession& operator=(CrawlSession&&) = delete;
};

class CrawlerManager {
public:
    CrawlerManager(std::shared_ptr<search_engine::storage::ContentStorage> storage);
    ~CrawlerManager();
    
    // Start a new crawl session
    std::string startCrawl(const std::string& url, const CrawlConfig& config, bool force = false);
    
    // Get crawl results by session ID
    std::vector<CrawlResult> getCrawlResults(const std::string& sessionId);
    
    // Get crawl status by session ID
    std::string getCrawlStatus(const std::string& sessionId);
    
    // Stop a specific crawl session
    bool stopCrawl(const std::string& sessionId);
    
    // Get all active sessions
    std::vector<std::string> getActiveSessions();
    
    // Clean up completed sessions (called periodically)
    void cleanupCompletedSessions();
    
    // Get session count for monitoring
    size_t getActiveSessionCount();

private:
    std::shared_ptr<search_engine::storage::ContentStorage> storage_;
    std::unordered_map<std::string, std::unique_ptr<CrawlSession>> sessions_;
    std::mutex sessionsMutex_;
    std::atomic<uint64_t> sessionCounter_{0};
    
    // Background cleanup thread
    std::thread cleanupThread_;
    std::atomic<bool> shouldStop_{false};
    
    // Generate unique session ID
    std::string generateSessionId();
    
    // Background cleanup worker
    void cleanupWorker();
    
    // Create a new crawler instance with configuration
    std::unique_ptr<Crawler> createCrawler(const CrawlConfig& config, const std::string& sessionId = "");
}; 