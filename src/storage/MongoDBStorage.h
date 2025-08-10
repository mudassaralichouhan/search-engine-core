#pragma once

#include "../../include/search_engine/storage/MongoDBStorage.h"
#include "../../include/search_engine/crawler/FrontierPersistence.h"

namespace search_engine { namespace storage {

class MongoFrontierPersistence : public search_engine::crawler::FrontierPersistence {
public:
    explicit MongoFrontierPersistence(MongoDBStorage* mongo) : mongo_(mongo) {}
    void upsertTask(const std::string& sessionId,
                    const std::string& url,
                    const std::string& normalizedUrl,
                    const std::string& domain,
                    int depth,
                    int priority,
                    const std::string& status,
                    const std::chrono::system_clock::time_point& readyAt,
                    int retryCount) override {
        if (mongo_) mongo_->frontierUpsertTask(sessionId, url, normalizedUrl, domain, depth, priority, status, readyAt, retryCount);
    }
    void markCompleted(const std::string& sessionId,
                       const std::string& normalizedUrl) override {
        if (mongo_) mongo_->frontierMarkCompleted(sessionId, normalizedUrl);
    }
    void updateRetry(const std::string& sessionId,
                     const std::string& normalizedUrl,
                     int retryCount,
                     const std::chrono::system_clock::time_point& nextReadyAt) override {
        if (mongo_) mongo_->frontierUpdateRetry(sessionId, normalizedUrl, retryCount, nextReadyAt);
    }
private:
    MongoDBStorage* mongo_;
};

} } // namespace search_engine::storage


