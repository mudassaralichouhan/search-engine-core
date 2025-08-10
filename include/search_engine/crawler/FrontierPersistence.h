#pragma once

#include <string>
#include <chrono>

namespace search_engine { namespace crawler {

class FrontierPersistence {
public:
    virtual ~FrontierPersistence() = default;

    virtual void upsertTask(const std::string& sessionId,
                            const std::string& url,
                            const std::string& normalizedUrl,
                            const std::string& domain,
                            int depth,
                            int priority,
                            const std::string& status,
                            const std::chrono::system_clock::time_point& readyAt,
                            int retryCount) = 0;

    virtual void markCompleted(const std::string& sessionId,
                               const std::string& normalizedUrl) = 0;

    virtual void updateRetry(const std::string& sessionId,
                             const std::string& normalizedUrl,
                             int retryCount,
                             const std::chrono::system_clock::time_point& nextReadyAt) = 0;
};

} } // namespace search_engine::crawler


