#include "RobotsTxtParser.h"
#include "../../include/Logger.h"
#include <sstream>
#include <algorithm>
#include <cctype>

RobotsTxtParser::RobotsTxtParser() {
    LOG_DEBUG("RobotsTxtParser constructor called");
}

RobotsTxtParser::~RobotsTxtParser() {
    LOG_DEBUG("RobotsTxtParser destructor called");
}

void RobotsTxtParser::parseRobotsTxt(const std::string& domain, const std::string& content) {
    LOG_INFO("RobotsTxtParser::parseRobotsTxt called for domain: " + domain);
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
            LOG_DEBUG("Parsed User-Agent: " + currentUserAgent);
            continue;
        }
        
        // Parse other directives
        RobotsRule& rule = (currentUserAgent == "*") ? 
            rules.defaultRules : rules.userAgentRules[currentUserAgent];
        
        parseLine(line, rule);
    }
    
    LOG_INFO("Finished parsing robots.txt for domain: " + domain);
}

bool RobotsTxtParser::isAllowed(const std::string& url, const std::string& userAgent) const {
    LOG_DEBUG("RobotsTxtParser::isAllowed called for URL: " + url + " with userAgent: " + userAgent);
    std::lock_guard<std::mutex> lock(rulesMutex);
    
    // Extract domain from URL
    size_t protocolEnd = url.find("://");
    if (protocolEnd == std::string::npos) {
        LOG_DEBUG("URL does not have a protocol, allowing: " + url);
        return true;
    }
    
    size_t domainStart = protocolEnd + 3;
    size_t domainEnd = url.find('/', domainStart);
    std::string domain = (domainEnd == std::string::npos) ? 
        url.substr(domainStart) : url.substr(domainStart, domainEnd - domainStart);
    
    LOG_TRACE("Extracted domain: " + domain + " from URL: " + url);
    
    auto it = domainRules.find(domain);
    if (it == domainRules.end()) {
        LOG_DEBUG("No robots.txt rules for domain: " + domain + ", allowing URL: " + url);
        return true;
    }
    
    const DomainRules& rules = it->second;
    
    // Check specific user agent rules first
    auto userAgentIt = rules.userAgentRules.find(userAgent);
    if (userAgentIt != rules.userAgentRules.end()) {
        const RobotsRule& rule = userAgentIt->second;
        
        // Check allow patterns first
        if (matchesPattern(url, rule.allowPatterns)) {
            LOG_DEBUG("URL explicitly allowed by specific user-agent rule: " + url);
            return true;
        }
        
        // Then check disallow patterns
        if (matchesPattern(url, rule.disallowPatterns)) {
            LOG_INFO("URL explicitly disallowed by specific user-agent rule: " + url);
            return false;
        }
    }
    
    // Check default rules
    const RobotsRule& defaultRule = rules.defaultRules;
    
    // Check allow patterns first
    if (matchesPattern(url, defaultRule.allowPatterns)) {
        LOG_DEBUG("URL explicitly allowed by default rule: " + url);
        return true;
    }
    
    // Then check disallow patterns
    if (matchesPattern(url, defaultRule.disallowPatterns)) {
        LOG_INFO("URL explicitly disallowed by default rule: " + url);
        return false;
    }
    
    LOG_DEBUG("No matching rules found for URL, allowing by default: " + url);
    return true;
}

std::chrono::milliseconds RobotsTxtParser::getCrawlDelay(const std::string& domain, const std::string& userAgent) const {
    LOG_DEBUG("RobotsTxtParser::getCrawlDelay called for domain: " + domain + " with userAgent: " + userAgent);
    std::lock_guard<std::mutex> lock(rulesMutex);
    
    auto it = domainRules.find(domain);
    if (it == domainRules.end()) {
        LOG_DEBUG("No robots.txt rules for domain: " + domain + ", using default crawl delay of 100ms");
        return std::chrono::milliseconds(100);
    }
    
    const DomainRules& rules = it->second;
    
    // Check specific user agent rules first
    auto userAgentIt = rules.userAgentRules.find(userAgent);
    if (userAgentIt != rules.userAgentRules.end()) {
        LOG_DEBUG("Found specific crawl delay for userAgent: " + userAgent + " = " + std::to_string(userAgentIt->second.crawlDelay.count()) + "ms");
        return userAgentIt->second.crawlDelay;
    }
    
    // Return default rules crawl delay
    LOG_DEBUG("Using default crawl delay: " + std::to_string(rules.defaultRules.crawlDelay.count()) + "ms");
    return rules.defaultRules.crawlDelay;
}

void RobotsTxtParser::clearCache(const std::string& domain) {
    LOG_DEBUG("RobotsTxtParser::clearCache called for domain: " + domain);
    std::lock_guard<std::mutex> lock(rulesMutex);
    domainRules.erase(domain);
}

bool RobotsTxtParser::isCached(const std::string& domain) const {
    std::lock_guard<std::mutex> lock(rulesMutex);
    bool cached = domainRules.find(domain) != domainRules.end();
    LOG_DEBUG("RobotsTxtParser::isCached called for domain: " + domain + " Result: " + (cached ? "cached" : "not cached"));
    return cached;
}

void RobotsTxtParser::parseLine(const std::string& line, RobotsRule& rule) {
    LOG_DEBUG("RobotsTxtParser::parseLine called with line: " + line);
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
            LOG_DEBUG("Added disallow pattern: " + pattern + " as regex: " + regexPattern);
        } else {
            LOG_DEBUG("Empty disallow pattern (allow all)");
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
            LOG_DEBUG("Added allow pattern: " + pattern + " as regex: " + regexPattern);
        } else {
            LOG_DEBUG("Empty allow pattern (ignored)");
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
            LOG_INFO("Set crawl delay to: " + std::to_string(rule.crawlDelay.count()) + "ms");
        }
        catch (...) {
            // Invalid crawl delay, keep default
            LOG_WARNING("Invalid crawl delay: " + delayStr + ", keeping default");
        }
    } else {
        LOG_DEBUG("Unrecognized directive in robots.txt: " + line);
    }
}

bool RobotsTxtParser::matchesPattern(const std::string& url, const std::vector<std::regex>& patterns) const {
    LOG_TRACE("RobotsTxtParser::matchesPattern called for URL: " + url + " with " + std::to_string(patterns.size()) + " patterns");
    for (const auto& pattern : patterns) {
        if (std::regex_search(url, pattern)) {
            LOG_DEBUG("URL: " + url + " matches a pattern");
            return true;
        }
    }
    LOG_DEBUG("URL: " + url + " does not match any patterns");
    return false;
} 