#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <curl/curl.h>
#include "FailureType.h"

struct CrawlResult {
    std::string url;
    std::string finalUrl;
    int statusCode;
    std::string contentType;
    std::optional<std::string> rawContent;
    std::optional<std::string> textContent;
    std::optional<std::string> title;
    std::optional<std::string> metaDescription;
    std::vector<std::string> links;
    std::chrono::system_clock::time_point queuedAt;
    std::chrono::system_clock::time_point startedAt;
    std::chrono::system_clock::time_point finishedAt;
    std::string crawlStatus;
    std::string domain;
    std::chrono::system_clock::time_point crawlTime;
    size_t contentSize;
    bool success;
    std::optional<std::string> errorMessage;

    CURLcode curlErrorCode = CURLE_OK;
    FailureType failureType = FailureType::UNKNOWN;
    int retryCount = 0;
    bool isRetryAttempt = false;
    std::chrono::milliseconds totalRetryTime{0};
};


