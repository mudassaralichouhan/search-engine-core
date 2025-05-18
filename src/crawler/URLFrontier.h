#pragma once

#include <string>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <chrono>

class URLFrontier {
public:
    URLFrontier();
    ~URLFrontier();

    // Add a URL to the frontier
    void addURL(const std::string& url);
    
    // Get the next URL to crawl
    std::string getNextURL();
    
    // Check if the frontier is empty
    bool isEmpty() const;
    
    // Get the number of URLs in the frontier
    size_t size() const;
    
    // Mark a URL as visited
    void markVisited(const std::string& url);
    
    // Check if a URL has been visited
    bool isVisited(const std::string& url) const;
    
    // Get the last visit time for a domain
    std::chrono::system_clock::time_point getLastVisitTime(const std::string& domain) const;

private:
    // Extract domain from URL
    std::string extractDomain(const std::string& url) const;
    
    // Normalize URL
    std::string normalizeURL(const std::string& url) const;

    std::queue<std::string> urlQueue;
    std::unordered_set<std::string> visitedURLs;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> domainLastVisit;
    
    mutable std::mutex queueMutex;
    mutable std::mutex visitedMutex;
    mutable std::mutex domainMutex;
}; 