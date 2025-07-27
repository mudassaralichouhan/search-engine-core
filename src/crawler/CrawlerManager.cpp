#include "CrawlerManager.h"
#include "../../include/Logger.h"
#include "PageFetcher.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

CrawlerManager::CrawlerManager(std::shared_ptr<search_engine::storage::ContentStorage> storage)
    : storage_(storage) {
    LOG_INFO("CrawlerManager initialized");
    
    // Start background cleanup thread
    cleanupThread_ = std::thread(&CrawlerManager::cleanupWorker, this);
}

CrawlerManager::~CrawlerManager() {
    LOG_INFO("CrawlerManager shutting down");
    
    // Signal cleanup thread to stop
    shouldStop_ = true;
    
    // Wait for cleanup thread to finish
    if (cleanupThread_.joinable()) {
        cleanupThread_.join();
    }
    
    // Stop all active crawls
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    for (auto& [sessionId, session] : sessions_) {
        if (session->crawler) {
            session->crawler->stop();
        }
        if (session->crawlThread.joinable()) {
            session->crawlThread.join();
        }
    }
    sessions_.clear();
    
    LOG_INFO("CrawlerManager shutdown complete");
}

std::string CrawlerManager::startCrawl(const std::string& url, const CrawlConfig& config, bool force) {
    std::string sessionId = generateSessionId();
    
    LOG_INFO("Starting new crawl session: " + sessionId + " for URL: " + url);
    
    try {
        // Create new crawler instance with the provided configuration
        auto crawler = createCrawler(config);
        
        // Create crawl session
        auto session = std::make_unique<CrawlSession>(sessionId, std::move(crawler));
        
        // Add seed URL to the crawler
        session->crawler->addSeedURL(url, force);
        
        // Start crawling in a separate thread
        session->crawlThread = std::thread([sessionId, this]() {
            std::unique_lock<std::mutex> lock(sessionsMutex_);
            auto it = sessions_.find(sessionId);
            if (it == sessions_.end()) {
                LOG_ERROR("Session not found during crawl start: " + sessionId);
                return;
            }
            
            auto& session = it->second;
            auto crawler = session->crawler.get();
            lock.unlock();
            
            try {
                LOG_INFO("Starting crawler for session: " + sessionId);
                crawler->start();
                
                // Wait for crawling to complete
                // The crawler will set isRunning to false when done
                while (crawler->getConfig().maxPages > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    
                    // Check if we should stop
                    lock.lock();
                    if (sessions_.find(sessionId) == sessions_.end()) {
                        lock.unlock();
                        break;
                    }
                    lock.unlock();
                    
                    // Check if crawler is still running by looking at results
                    auto results = crawler->getResults();
                    bool hasActiveDownloads = false;
                    for (const auto& result : results) {
                        if (result.crawlStatus == "queued" || result.crawlStatus == "downloading") {
                            hasActiveDownloads = true;
                            break;
                        }
                    }
                    
                    if (!hasActiveDownloads && !results.empty()) {
                        LOG_INFO("No more active downloads for session: " + sessionId);
                        break;
                    }
                    
                    if (results.size() >= crawler->getConfig().maxPages) {
                        LOG_INFO("Reached max pages for session: " + sessionId);
                        break;
                    }
                }
                
                LOG_INFO("Crawl completed for session: " + sessionId);
                
            } catch (const std::exception& e) {
                LOG_ERROR("Error in crawl thread for session " + sessionId + ": " + e.what());
            }
            
            // Mark session as completed
            lock.lock();
            auto sessionIt = sessions_.find(sessionId);
            if (sessionIt != sessions_.end()) {
                sessionIt->second->isCompleted = true;
            }
            lock.unlock();
        });
        
        // Store session
        {
            std::lock_guard<std::mutex> lock(sessionsMutex_);
            sessions_[sessionId] = std::move(session);
        }
        
        LOG_INFO("Crawl session started successfully: " + sessionId);
        return sessionId;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to start crawl session: " + std::string(e.what()));
        throw;
    }
}

std::vector<CrawlResult> CrawlerManager::getCrawlResults(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        LOG_WARNING("Session not found: " + sessionId);
        return {};
    }
    
    return it->second->crawler->getResults();
}

std::string CrawlerManager::getCrawlStatus(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        return "not_found";
    }
    
    if (it->second->isCompleted) {
        return "completed";
    }
    
    auto results = it->second->crawler->getResults();
    if (results.empty()) {
        return "starting";
    }
    
    bool hasActive = false;
    for (const auto& result : results) {
        if (result.crawlStatus == "queued" || result.crawlStatus == "downloading") {
            hasActive = true;
            break;
        }
    }
    
    return hasActive ? "crawling" : "completed";
}

bool CrawlerManager::stopCrawl(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) {
        return false;
    }
    
    LOG_INFO("Stopping crawl session: " + sessionId);
    
    if (it->second->crawler) {
        it->second->crawler->stop();
    }
    
    it->second->isCompleted = true;
    return true;
}

std::vector<std::string> CrawlerManager::getActiveSessions() {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    
    std::vector<std::string> activeSessionIds;
    for (const auto& [sessionId, session] : sessions_) {
        if (!session->isCompleted) {
            activeSessionIds.push_back(sessionId);
        }
    }
    
    return activeSessionIds;
}

void CrawlerManager::cleanupCompletedSessions() {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    
    auto now = std::chrono::system_clock::now();
    auto it = sessions_.begin();
    
    while (it != sessions_.end()) {
        const auto& session = it->second;
        
        // Clean up completed sessions older than 5 minutes
        bool shouldCleanup = session->isCompleted && 
            (now - session->createdAt) > std::chrono::minutes(5);
        
        if (shouldCleanup) {
            LOG_INFO("Cleaning up completed session: " + session->id);
            
            // Stop crawler if still running
            if (session->crawler) {
                session->crawler->stop();
            }
            
            // Wait for thread to complete
            if (session->crawlThread.joinable()) {
                // Unlock temporarily to allow thread to complete
                sessionsMutex_.unlock();
                session->crawlThread.join();
                sessionsMutex_.lock();
            }
            
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

size_t CrawlerManager::getActiveSessionCount() {
    std::lock_guard<std::mutex> lock(sessionsMutex_);
    return sessions_.size();
}

std::string CrawlerManager::generateSessionId() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    auto counter = sessionCounter_.fetch_add(1);
    
    std::stringstream ss;
    ss << "crawl_" << timestamp << "_" << counter;
    return ss.str();
}

void CrawlerManager::cleanupWorker() {
    LOG_INFO("CrawlerManager cleanup worker started");
    
    while (!shouldStop_) {
        try {
            cleanupCompletedSessions();
            
            // Sleep for 30 seconds
            for (int i = 0; i < 300 && !shouldStop_; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR("Error in cleanup worker: " + std::string(e.what()));
        }
    }
    
    LOG_INFO("CrawlerManager cleanup worker stopped");
}

std::unique_ptr<Crawler> CrawlerManager::createCrawler(const CrawlConfig& config) {
    auto crawler = std::make_unique<Crawler>(config, storage_);
    
    // Configure PageFetcher settings
    if (crawler->getPageFetcher()) {
        // Disable SSL verification for problematic sites
        crawler->getPageFetcher()->setVerifySSL(false);
        
        // Enable SPA rendering if configured
        if (config.spaRenderingEnabled) {
            crawler->getPageFetcher()->setSpaRendering(true, config.browserlessUrl);
        }
    }
    
    return crawler;
} 