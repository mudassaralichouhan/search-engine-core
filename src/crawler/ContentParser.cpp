#include "ContentParser.h"
#include <algorithm>
#include <regex>
#include <sstream>
#include <cctype>

ContentParser::ContentParser() {}
ContentParser::~ContentParser() {}

ParsedContent ContentParser::parse(const std::string& html, const std::string& baseUrl) {
    ParsedContent result;
    
    // Parse HTML using Gumbo
    GumboOutput* output = gumbo_parse(html.c_str());
    
    if (output) {
        // Extract title
        result.title = extractTitle(html).value_or("");
        
        // Extract meta description
        result.metaDescription = extractMetaDescription(html).value_or("");
        
        // Extract text content
        result.textContent = extractText(html);
        
        // Extract links
        result.links = extractLinks(html, baseUrl);
        
        // Clean up Gumbo output
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
    
    return result;
}

std::string ContentParser::extractText(const std::string& html) {
    std::string text;
    GumboOutput* output = gumbo_parse(html.c_str());
    
    if (output) {
        extractTextFromNode(output->root, text);
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
    
    return text;
}

std::vector<std::string> ContentParser::extractLinks(const std::string& html, const std::string& baseUrl) {
    std::vector<std::string> links;
    GumboOutput* output = gumbo_parse(html.c_str());
    
    if (output) {
        extractLinksFromNode(output->root, baseUrl, links);
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
    
    return links;
}

std::optional<std::string> ContentParser::extractTitle(const std::string& html) {
    GumboOutput* output = gumbo_parse(html.c_str());
    std::optional<std::string> title;
    
    if (output) {
        GumboNode* root = output->root;
        GumboNode* head = nullptr;
        
        // Find the head node
        for (unsigned int i = 0; i < root->v.element.children.length; ++i) {
            GumboNode* child = static_cast<GumboNode*>(root->v.element.children.data[i]);
            if (child->type == GUMBO_NODE_ELEMENT && child->v.element.tag == GUMBO_TAG_HEAD) {
                head = child;
                break;
            }
        }
        
        if (head) {
            // Find the title node
            for (unsigned int i = 0; i < head->v.element.children.length; ++i) {
                GumboNode* child = static_cast<GumboNode*>(head->v.element.children.data[i]);
                if (child->type == GUMBO_NODE_ELEMENT && child->v.element.tag == GUMBO_TAG_TITLE) {
                    if (child->v.element.children.length > 0) {
                        GumboNode* titleText = static_cast<GumboNode*>(child->v.element.children.data[0]);
                        if (titleText->type == GUMBO_NODE_TEXT) {
                            title = titleText->v.text.text;
                        }
                    }
                    break;
                }
            }
        }
        
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
    
    return title;
}

std::optional<std::string> ContentParser::extractMetaDescription(const std::string& html) {
    GumboOutput* output = gumbo_parse(html.c_str());
    std::optional<std::string> description;
    
    if (output) {
        description = findMetaTag(output->root, "description");
        gumbo_destroy_output(&kGumboDefaultOptions, output);
    }
    
    return description;
}

void ContentParser::extractTextFromNode(const GumboNode* node, std::string& text) {
    if (node->type == GUMBO_NODE_TEXT) {
        text += node->v.text.text;
        text += " ";
    }
    else if (node->type == GUMBO_NODE_ELEMENT) {
        // Skip script and style tags
        if (node->v.element.tag != GUMBO_TAG_SCRIPT && 
            node->v.element.tag != GUMBO_TAG_STYLE) {
            for (unsigned int i = 0; i < node->v.element.children.length; ++i) {
                extractTextFromNode(static_cast<GumboNode*>(node->v.element.children.data[i]), text);
            }
        }
    }
}

void ContentParser::extractLinksFromNode(const GumboNode* node, const std::string& baseUrl, std::vector<std::string>& links) {
    if (node->type == GUMBO_NODE_ELEMENT) {
        if (node->v.element.tag == GUMBO_TAG_A) {
            GumboAttribute* href = gumbo_get_attribute(&node->v.element.attributes, "href");
            if (href) {
                std::string url = normalizeUrl(href->value, baseUrl);
                if (isValidUrl(url)) {
                    links.push_back(url);
                }
            }
        }
        
        for (unsigned int i = 0; i < node->v.element.children.length; ++i) {
            extractLinksFromNode(static_cast<GumboNode*>(node->v.element.children.data[i]), baseUrl, links);
        }
    }
}

std::optional<std::string> ContentParser::findMetaTag(const GumboNode* node, const std::string& name) {
    if (node->type == GUMBO_NODE_ELEMENT) {
        if (node->v.element.tag == GUMBO_TAG_META) {
            GumboAttribute* nameAttr = gumbo_get_attribute(&node->v.element.attributes, "name");
            GumboAttribute* contentAttr = gumbo_get_attribute(&node->v.element.attributes, "content");
            
            if (nameAttr && contentAttr && nameAttr->value == name) {
                return contentAttr->value;
            }
        }
        
        for (unsigned int i = 0; i < node->v.element.children.length; ++i) {
            auto result = findMetaTag(static_cast<GumboNode*>(node->v.element.children.data[i]), name);
            if (result) {
                return result;
            }
        }
    }
    
    return std::nullopt;
}

std::string ContentParser::normalizeUrl(const std::string& url, const std::string& baseUrl) {
    if (url.empty()) {
        return "";
    }
    
    // If URL is absolute, return as is
    if (url.find("http://") == 0 || url.find("https://") == 0) {
        return url;
    }
    
    // If URL starts with //, add https:
    if (url.find("//") == 0) {
        return "https:" + url;
    }
    
    // If URL starts with /, append to base URL's domain
    if (url[0] == '/') {
        size_t protocolEnd = baseUrl.find("://");
        if (protocolEnd != std::string::npos) {
            size_t domainEnd = baseUrl.find('/', protocolEnd + 3);
            if (domainEnd != std::string::npos) {
                return baseUrl.substr(0, domainEnd) + url;
            }
        }
        return baseUrl + url;
    }
    
    // Otherwise, append to base URL's directory
    size_t lastSlash = baseUrl.find_last_of('/');
    if (lastSlash != std::string::npos) {
        return baseUrl.substr(0, lastSlash + 1) + url;
    }
    
    return baseUrl + "/" + url;
}

bool ContentParser::isValidUrl(const std::string& url) {
    static const std::regex urlRegex(
        R"(^(https?:\/\/)?([\da-z\.-]+)\.([a-z\.]{2,6})([\/\w \.-]*)*\/?$)"
    );
    return std::regex_match(url, urlRegex);
} 