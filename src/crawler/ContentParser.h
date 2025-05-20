#pragma once

#include <string>
#include <vector>
#include <optional>
#include <gumbo.h>

struct ParsedContent {
    std::string title;
    std::string metaDescription;
    std::string textContent;
    std::vector<std::string> links;
};

class ContentParser {
public:
    ContentParser();
    ~ContentParser();

    // Parse HTML content
    ParsedContent parse(const std::string& html, const std::string& baseUrl);
    
    // Extract text content from HTML
    std::string extractText(const std::string& html);
    
    // Extract links from HTML
    std::vector<std::string> extractLinks(const std::string& html, const std::string& baseUrl);
    
    // Extract title from HTML
    std::optional<std::string> extractTitle(const std::string& html);
    
    // Extract meta description from HTML
    std::optional<std::string> extractMetaDescription(const std::string& html);

    // Helper function to check if a URL is valid
    bool isValidUrl(const std::string& url);

private:
    // Helper function to extract text from a GumboNode
    void extractTextFromNode(const GumboNode* node, std::string& text);
    
    // Helper function to extract links from a GumboNode
    void extractLinksFromNode(const GumboNode* node, const std::string& baseUrl, std::vector<std::string>& links);
    
    // Helper function to find a specific meta tag
    std::optional<std::string> findMetaTag(const GumboNode* node, const std::string& name);
    
    // Helper function to normalize URL
    std::string normalizeUrl(const std::string& url, const std::string& baseUrl);
}; 