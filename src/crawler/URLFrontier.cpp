#include "URLFrontier.h"
#include "../../include/Logger.h"
#include <algorithm>
#include <regex>
#include <sstream>
#include <iomanip>

URLFrontier::URLFrontier() {
    LOG_DEBUG("URLFrontier constructor called");
}

URLFrontier::~URLFrontier() {
    LOG_DEBUG("URLFrontier destructor called");
}

void URLFrontier::addURL(const std::string& url, bool force) {
    LOG_DEBUG("URLFrontier::addURL called with: " + url + (force ? " (force)" : ""));
    std::string normalizedURL = normalizeURL(url);
    std::string domain = extractDomain(normalizedURL);
    if (force) {
        // Remove from visited set if present
        {
            std::lock_guard<std::mutex> visitedLock(visitedMutex);
            visitedURLs.erase(normalizedURL);
        }
        // Remove from queue if present
        {
            std::lock_guard<std::mutex> queueLock(queueMutex);
            std::queue<std::string> tempQueue;
            while (!urlQueue.empty()) {
                std::string current = urlQueue.front();
                urlQueue.pop();
                if (current != normalizedURL) {
                    tempQueue.push(current);
                }
            }
            while (!tempQueue.empty()) {
                urlQueue.push(tempQueue.front());
                tempQueue.pop();
            }
        }
    }
    // Check both visited URLs and queue for duplicates
    {
        std::lock_guard<std::mutex> visitedLock(visitedMutex);
        if (!force && visitedURLs.find(normalizedURL) != visitedURLs.end()) {
            LOG_DEBUG("URL already visited, skipping: " + normalizedURL);
            return;
        }
    }
    {
        std::lock_guard<std::mutex> queueLock(queueMutex);
        if (!force) {
            std::queue<std::string> tempQueue;
            bool found = false;
            while (!urlQueue.empty()) {
                std::string current = urlQueue.front();
                urlQueue.pop();
                if (current == normalizedURL) {
                    found = true;
                }
                tempQueue.push(current);
            }
            while (!tempQueue.empty()) {
                urlQueue.push(tempQueue.front());
                tempQueue.pop();
            }
            if (found) {
                LOG_DEBUG("URL already in queue, skipping: " + normalizedURL);
                return;
            }
        }
        urlQueue.push(normalizedURL);
        LOG_DEBUG("Added URL to queue: " + normalizedURL + ", queue size: " + std::to_string(urlQueue.size()));
    }
}

std::string URLFrontier::getNextURL() {
    std::lock_guard<std::mutex> lock(queueMutex);
    if (urlQueue.empty()) {
        LOG_DEBUG("URLFrontier::getNextURL - Queue is empty, returning empty string");
        return "";
    }
    
    std::string url = urlQueue.front();
    urlQueue.pop();
    LOG_DEBUG("URLFrontier::getNextURL - Retrieved URL: " + url + ", remaining queue size: " + std::to_string(urlQueue.size()));
    return url;
}

bool URLFrontier::isEmpty() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    bool empty = urlQueue.empty();
    LOG_TRACE("URLFrontier::isEmpty - Queue is " + std::string(empty ? "empty" : "not empty"));
    return empty;
}

size_t URLFrontier::size() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    size_t size = urlQueue.size();
    LOG_TRACE("URLFrontier::size - Queue size: " + std::to_string(size));
    return size;
}

void URLFrontier::markVisited(const std::string& url) {
    LOG_DEBUG("URLFrontier::markVisited called with: " + url);
    std::string normalizedURL = normalizeURL(url);
    std::string domain = extractDomain(normalizedURL);
    
    std::lock_guard<std::mutex> visitedLock(visitedMutex);
    visitedURLs.insert(normalizedURL);
    LOG_DEBUG("Marked URL as visited: " + normalizedURL + ", visited URLs count: " + std::to_string(visitedURLs.size()));
    
    std::lock_guard<std::mutex> domainLock(domainMutex);
    domainLastVisit[domain] = std::chrono::system_clock::now();
    LOG_DEBUG("Updated last visit time for domain: " + domain);
}

bool URLFrontier::isVisited(const std::string& url) const {
    std::string normalizedURL = normalizeURL(url);
    std::lock_guard<std::mutex> lock(visitedMutex);
    bool visited = visitedURLs.find(normalizedURL) != visitedURLs.end();
    LOG_TRACE("URLFrontier::isVisited - URL: " + url + " is " + (visited ? "visited" : "not visited"));
    return visited;
}

std::chrono::system_clock::time_point URLFrontier::getLastVisitTime(const std::string& domain) const {
    std::lock_guard<std::mutex> lock(domainMutex);
    auto it = domainLastVisit.find(domain);
    if (it != domainLastVisit.end()) {
        LOG_TRACE("URLFrontier::getLastVisitTime - Domain: " + domain + " has a last visit time");
        return it->second;
    }
    LOG_TRACE("URLFrontier::getLastVisitTime - Domain: " + domain + " has no last visit time");
    return std::chrono::system_clock::time_point::min();
}

std::string URLFrontier::extractDomain(const std::string& url) const {
    static const std::regex domainRegex(R"(https?://([^/:]+))");
    std::smatch matches;
    if (std::regex_search(url, matches, domainRegex) && matches.size() > 1) {
        std::string domain = matches[1].str();
        LOG_TRACE("URLFrontier::extractDomain - Extracted domain: " + domain + " from URL: " + url);
        return domain;
    }
    LOG_WARNING("URLFrontier::extractDomain - Could not extract domain from URL: " + url);
    return "";
}

std::string URLFrontier::normalizeURL(const std::string& url) const {
    std::string normalized = url;
    
    // Convert to lowercase
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove fragment
    size_t hashPos = normalized.find('#');
    if (hashPos != std::string::npos) {
        normalized = normalized.substr(0, hashPos);
    }
    
    // Handle trailing slash more intelligently
    // Only remove trailing slash if it's not a root URL (has path after domain)
    if (!normalized.empty() && normalized.back() == '/') {
        // Check if this is a root URL (no path after domain)
        size_t protocolEnd = normalized.find("://");
        if (protocolEnd != std::string::npos) {
            size_t domainEnd = normalized.find('/', protocolEnd + 3);
            if (domainEnd != std::string::npos && domainEnd == normalized.length() - 1) {
                // This is a root URL with trailing slash, keep it to avoid redirect loops
                LOG_TRACE("URLFrontier::normalizeURL - Keeping trailing slash for root URL: " + normalized);
            } else {
                // This has a path, remove trailing slash
                normalized.pop_back();
            }
        } else {
            // No protocol found, remove trailing slash
            normalized.pop_back();
        }
    }
    
    LOG_TRACE("URLFrontier::normalizeURL - Original: " + url + " Normalized: " + normalized);
    return normalized;
} 