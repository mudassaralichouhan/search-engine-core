#include "RobotsTxtParser.h"
#include <sstream>
#include <algorithm>
#include <cctype>

RobotsTxtParser::RobotsTxtParser() {}
RobotsTxtParser::~RobotsTxtParser() {}

void RobotsTxtParser::parseRobotsTxt(const std::string& domain, const std::string& content) {
    std::lock_guard<std::mutex> lock(rulesMutex);
    
    DomainRules& rules = domainRules[domain];
    rules.lastUpdated = std::chrono::system_clock::now();
    
    std::istringstream stream(content);
    std::string line;
    std::string currentUserAgent = "*";
    
    while (std::getline(stream, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Convert to lowercase for case-insensitive matching
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        
        // Parse User-agent line
        if (line.find("user-agent:") == 0) {
            currentUserAgent = line.substr(11);
            // Trim whitespace
            currentUserAgent.erase(0, currentUserAgent.find_first_not_of(" \t"));
            currentUserAgent.erase(currentUserAgent.find_last_not_of(" \t") + 1);
            continue;
        }
        
        // Parse other directives
        RobotsRule& rule = (currentUserAgent == "*") ? 
            rules.defaultRules : rules.userAgentRules[currentUserAgent];
        
        parseLine(line, rule);
    }
}

bool RobotsTxtParser::isAllowed(const std::string& url, const std::string& userAgent) const {
    std::lock_guard<std::mutex> lock(rulesMutex);
    
    // Extract domain from URL
    size_t protocolEnd = url.find("://");
    if (protocolEnd == std::string::npos) return true;
    
    size_t domainStart = protocolEnd + 3;
    size_t domainEnd = url.find('/', domainStart);
    std::string domain = (domainEnd == std::string::npos) ? 
        url.substr(domainStart) : url.substr(domainStart, domainEnd - domainStart);
    
    auto it = domainRules.find(domain);
    if (it == domainRules.end()) return true;
    
    const DomainRules& rules = it->second;
    
    // Check specific user agent rules first
    auto userAgentIt = rules.userAgentRules.find(userAgent);
    if (userAgentIt != rules.userAgentRules.end()) {
        const RobotsRule& rule = userAgentIt->second;
        
        // Check allow patterns first
        if (matchesPattern(url, rule.allowPatterns)) {
            return true;
        }
        
        // Then check disallow patterns
        if (matchesPattern(url, rule.disallowPatterns)) {
            return false;
        }
    }
    
    // Check default rules
    const RobotsRule& defaultRule = rules.defaultRules;
    
    // Check allow patterns first
    if (matchesPattern(url, defaultRule.allowPatterns)) {
        return true;
    }
    
    // Then check disallow patterns
    if (matchesPattern(url, defaultRule.disallowPatterns)) {
        return false;
    }
    
    return true;
}

std::chrono::milliseconds RobotsTxtParser::getCrawlDelay(const std::string& domain, const std::string& userAgent) const {
    std::lock_guard<std::mutex> lock(rulesMutex);
    
    auto it = domainRules.find(domain);
    if (it == domainRules.end()) return std::chrono::milliseconds(1000);
    
    const DomainRules& rules = it->second;
    
    // Check specific user agent rules first
    auto userAgentIt = rules.userAgentRules.find(userAgent);
    if (userAgentIt != rules.userAgentRules.end()) {
        return userAgentIt->second.crawlDelay;
    }
    
    // Return default rules crawl delay
    return rules.defaultRules.crawlDelay;
}

void RobotsTxtParser::clearCache(const std::string& domain) {
    std::lock_guard<std::mutex> lock(rulesMutex);
    domainRules.erase(domain);
}

bool RobotsTxtParser::isCached(const std::string& domain) const {
    std::lock_guard<std::mutex> lock(rulesMutex);
    return domainRules.find(domain) != domainRules.end();
}

void RobotsTxtParser::parseLine(const std::string& line, RobotsRule& rule) {
    if (line.find("disallow:") == 0) {
        std::string pattern = line.substr(9);
        // Trim whitespace
        pattern.erase(0, pattern.find_first_not_of(" \t"));
        pattern.erase(pattern.find_last_not_of(" \t") + 1);
        
        if (!pattern.empty()) {
            // Convert glob pattern to regex
            std::string regexPattern = std::regex_replace(pattern, 
                std::regex("\\*"), ".*");
            regexPattern = std::regex_replace(regexPattern, 
                std::regex("\\?"), ".");
            rule.disallowPatterns.push_back(std::regex(regexPattern));
        }
    }
    else if (line.find("allow:") == 0) {
        std::string pattern = line.substr(6);
        // Trim whitespace
        pattern.erase(0, pattern.find_first_not_of(" \t"));
        pattern.erase(pattern.find_last_not_of(" \t") + 1);
        
        if (!pattern.empty()) {
            // Convert glob pattern to regex
            std::string regexPattern = std::regex_replace(pattern, 
                std::regex("\\*"), ".*");
            regexPattern = std::regex_replace(regexPattern, 
                std::regex("\\?"), ".");
            rule.allowPatterns.push_back(std::regex(regexPattern));
        }
    }
    else if (line.find("crawl-delay:") == 0) {
        std::string delayStr = line.substr(12);
        // Trim whitespace
        delayStr.erase(0, delayStr.find_first_not_of(" \t"));
        delayStr.erase(delayStr.find_last_not_of(" \t") + 1);
        
        try {
            float delay = std::stof(delayStr);
            rule.crawlDelay = std::chrono::milliseconds(
                static_cast<int>(delay * 1000));
        }
        catch (...) {
            // Invalid crawl delay, keep default
        }
    }
}

bool RobotsTxtParser::matchesPattern(const std::string& url, const std::vector<std::regex>& patterns) const {
    for (const auto& pattern : patterns) {
        if (std::regex_search(url, pattern)) {
            return true;
        }
    }
    return false;
} 