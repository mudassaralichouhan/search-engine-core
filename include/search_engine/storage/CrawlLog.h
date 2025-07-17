#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>

namespace search_engine {
namespace storage {

struct CrawlLog {
    std::optional<std::string> id; // MongoDB ObjectId
    std::string url;
    std::string domain;
    std::chrono::system_clock::time_point crawlTime;
    std::string status; // e.g., "SUCCESS", "FAILED"
    int httpStatusCode;
    std::optional<std::string> errorMessage;
    size_t contentSize;
    std::string contentType;
    std::vector<std::string> links;
    std::optional<std::string> title;
    std::optional<std::string> description;
};

} // namespace storage
} // namespace search_engine 