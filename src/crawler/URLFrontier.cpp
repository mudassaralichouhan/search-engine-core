#include "URLFrontier.h"
#include <algorithm>
#include <regex>
#include <sstream>
#include <iomanip>

URLFrontier::URLFrontier() {}
URLFrontier::~URLFrontier() {}

void URLFrontier::addURL(const std::string& url) {
    std::string normalizedURL = normalizeURL(url);
    std::string domain = extractDomain(normalizedURL);
    
    std::lock_guard<std::mutex> visitedLock(visitedMutex);
    if (visitedURLs.find(normalizedURL) != visitedURLs.end()) {
        return;
    }
    
    std::lock_guard<std::mutex> queueLock(queueMutex);
    urlQueue.push(normalizedURL);
}

std::string URLFrontier::getNextURL() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (urlQueue.empty()) {
        return "";
    }
    
    std::string url = urlQueue.front();
    urlQueue.pop();
    return url;
}

bool URLFrontier::isEmpty() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return urlQueue.empty();
}

size_t URLFrontier::size() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return urlQueue.size();
}

void URLFrontier::markVisited(const std::string& url) {
    std::string normalizedURL = normalizeURL(url);
    std::string domain = extractDomain(normalizedURL);
    
    std::lock_guard<std::mutex> visitedLock(visitedMutex);
    visitedURLs.insert(normalizedURL);
    
    std::lock_guard<std::mutex> domainLock(domainMutex);
    domainLastVisit[domain] = std::chrono::system_clock::now();
}

bool URLFrontier::isVisited(const std::string& url) const {
    std::string normalizedURL = normalizeURL(url);
    std::lock_guard<std::mutex> lock(visitedMutex);
    return visitedURLs.find(normalizedURL) != visitedURLs.end();
}

std::chrono::system_clock::time_point URLFrontier::getLastVisitTime(const std::string& domain) const {
    std::lock_guard<std::mutex> lock(domainMutex);
    auto it = domainLastVisit.find(domain);
    if (it != domainLastVisit.end()) {
        return it->second;
    }
    return std::chrono::system_clock::time_point::min();
}

std::string URLFrontier::extractDomain(const std::string& url) const {
    static const std::regex domainRegex(R"(https?://([^/]+))");
    std::smatch matches;
    if (std::regex_search(url, matches, domainRegex) && matches.size() > 1) {
        return matches[1].str();
    }
    return "";
}

std::string URLFrontier::normalizeURL(const std::string& url) const {
    std::string normalized = url;
    
    // Convert to lowercase
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove trailing slash
    if (!normalized.empty() && normalized.back() == '/') {
        normalized.pop_back();
    }
    
    // Remove fragment
    size_t hashPos = normalized.find('#');
    if (hashPos != std::string::npos) {
        normalized = normalized.substr(0, hashPos);
    }
    
    return normalized;
} 