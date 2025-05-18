#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <regex>
#include <mutex>
#include <chrono>

class RobotsTxtParser {
public:
    RobotsTxtParser();
    ~RobotsTxtParser();

    // Parse robots.txt content for a domain
    void parseRobotsTxt(const std::string& domain, const std::string& content);
    
    // Check if a URL is allowed to be crawled
    bool isAllowed(const std::string& url, const std::string& userAgent) const;
    
    // Get the crawl delay for a domain
    std::chrono::milliseconds getCrawlDelay(const std::string& domain, const std::string& userAgent) const;
    
    // Clear cached robots.txt for a domain
    void clearCache(const std::string& domain);
    
    // Check if robots.txt is cached for a domain
    bool isCached(const std::string& domain) const;

private:
    struct RobotsRule {
        std::vector<std::regex> disallowPatterns;
        std::vector<std::regex> allowPatterns;
        std::chrono::milliseconds crawlDelay{1000}; // Default 1 second
    };

    struct DomainRules {
        std::unordered_map<std::string, RobotsRule> userAgentRules;
        RobotsRule defaultRules;
        std::chrono::system_clock::time_point lastUpdated;
    };

    // Parse a single robots.txt line
    void parseLine(const std::string& line, RobotsRule& rule);
    
    // Check if a URL matches any pattern
    bool matchesPattern(const std::string& url, const std::vector<std::regex>& patterns) const;

    std::unordered_map<std::string, DomainRules> domainRules;
    mutable std::mutex rulesMutex;
}; 