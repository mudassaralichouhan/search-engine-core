#include "search_core/QueryParser.hpp"
#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <iomanip>

namespace hatef::search {

std::unique_ptr<Node> QueryParser::parse(const std::string& query) {
    if (query.empty()) {
        return std::make_unique<Term>("");
    }
    
    auto normalized = normalize(query);
    auto tokens = tokenize(normalized);
    return parseTokens(tokens);
}

std::string QueryParser::normalize(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    
    bool in_quotes = false;
    for (char c : text) {
        if (c == '"') {
            in_quotes = !in_quotes;
            result += c;
        } else if (in_quotes) {
            // Preserve everything inside quotes
            result += c;
        } else {
            // Convert to lowercase and handle punctuation
            if (std::isalnum(c) || c == ':' || c == '-' || c == '.' || c == ' ') {
                result += static_cast<char>(std::tolower(c));
            } else {
                result += ' ';
            }
        }
    }
    
    return result;
}

std::vector<std::string> QueryParser::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string current_token;
    bool in_quotes = false;
    
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        
        if (c == '"' && !in_quotes) {
            // Start of quoted string
            in_quotes = true;
            current_token = "\"";
        } else if (c == '"' && in_quotes) {
            // End of quoted string
            current_token += "\"";
            tokens.push_back(current_token);
            current_token.clear();
            in_quotes = false;
        } else if (in_quotes) {
            // Inside quotes, preserve everything
            current_token += c;
        } else if (std::isspace(c)) {
            // Outside quotes, whitespace separates tokens
            if (!current_token.empty()) {
                // Clean up tokens - remove trailing colons from malformed filters
                if (current_token.back() == ':') {
                    // Check if this is a trailing colon (i.e., colon at the end with no value after)
                    auto colon_pos = current_token.find(':');
                    if (colon_pos == current_token.length() - 1) {
                        current_token.pop_back();
                    }
                }
                if (!current_token.empty()) {
                    tokens.push_back(current_token);
                }
                current_token.clear();
            }
        } else {
            // Regular character
            current_token += c;
        }
    }
    
    // Add final token if any
    if (!current_token.empty()) {
        // Clean up final token too
        if (current_token.back() == ':') {
            // Check if this is a trailing colon (i.e., colon at the end with no value after)
            auto colon_pos = current_token.find(':');
            if (colon_pos == current_token.length() - 1) {
                current_token.pop_back();
            }
        }
        if (!current_token.empty()) {
            tokens.push_back(current_token);
        }
    }
    
    return tokens;
}

std::unique_ptr<Node> QueryParser::parseTokens(const std::vector<std::string>& tokens) {
    if (tokens.empty()) {
        return std::make_unique<Term>("");
    }
    
    if (tokens.size() == 1) {
        const auto& token = tokens[0];
        
        // Check for quoted string
        if (token.front() == '"' && token.back() == '"' && token.size() > 1) {
            return std::make_unique<Term>(token.substr(1, token.size() - 2), true);
        }
        
        // Check for filter
        if (auto filter = parseFilter(token)) {
            return std::move(filter);
        }
        
        return std::make_unique<Term>(token);
    }
    
    // Handle multiple tokens - look for OR first, then group by AND
    std::vector<std::unique_ptr<Node>> or_groups;
    std::vector<std::unique_ptr<Node>> current_and_group;
    
    // First, collect all non-operator tokens and check if we have only operators
    std::vector<std::string> non_operators;
    for (const auto& token : tokens) {
        if (token != "and" && token != "AND" && token != "or" && token != "OR") {
            non_operators.push_back(token);
        }
    }
    
    // If we only have operators, treat them as regular terms
    if (non_operators.empty()) {
        for (const auto& token : tokens) {
            current_and_group.push_back(std::make_unique<Term>(token));
        }
        
        if (current_and_group.size() == 1) {
            return std::move(current_and_group[0]);
        } else {
            auto and_node = std::make_unique<And>();
            for (auto& node : current_and_group) {
                and_node->addChild(std::move(node));
            }
            return std::move(and_node);
        }
    }
    
    // Normal parsing logic for tokens with operands
    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        
        if (token == "or" || token == "OR") {
            // Finish current AND group and start new one
            if (!current_and_group.empty()) {
                if (current_and_group.size() == 1) {
                    or_groups.push_back(std::move(current_and_group[0]));
                } else {
                    auto and_node = std::make_unique<And>();
                    for (auto& node : current_and_group) {
                        and_node->addChild(std::move(node));
                    }
                    or_groups.push_back(std::move(and_node));
                }
                current_and_group.clear();
            }
        } else if (token == "and" || token == "AND") {
            // Skip explicit AND (it's default)
            continue;
        } else {
            std::unique_ptr<Node> node;
            
            // Check for quoted string
            if (token.front() == '"' && token.back() == '"' && token.size() > 1) {
                node = std::make_unique<Term>(token.substr(1, token.size() - 2), true);
            } else if (auto filter = parseFilter(token)) {
                node = std::move(filter);
            } else {
                node = std::make_unique<Term>(token);
            }
            
            current_and_group.push_back(std::move(node));
        }
    }
    
    // Add remaining AND group
    if (!current_and_group.empty()) {
        if (current_and_group.size() == 1) {
            or_groups.push_back(std::move(current_and_group[0]));
        } else {
            auto and_node = std::make_unique<And>();
            for (auto& node : current_and_group) {
                and_node->addChild(std::move(node));
            }
            or_groups.push_back(std::move(and_node));
        }
    }
    
    // Return final structure
    if (or_groups.empty()) {
        return std::make_unique<Term>("");
    } else if (or_groups.size() == 1) {
        return std::move(or_groups[0]);
    } else {
        auto or_node = std::make_unique<Or>();
        for (auto& group : or_groups) {
            or_node->addChild(std::move(group));
        }
        return std::move(or_node);
    }
}

bool QueryParser::isSpecialToken(const std::string& token) {
    return token.find(':') != std::string::npos ||
           token == "and" || token == "AND" ||
           token == "or" || token == "OR";
}

std::unique_ptr<Filter> QueryParser::parseFilter(const std::string& token) {
    auto colon_pos = token.find(':');
    if (colon_pos == std::string::npos || colon_pos == 0 || colon_pos == token.size() - 1) {
        return nullptr;
    }
    
    std::string field = token.substr(0, colon_pos);
    std::string value = token.substr(colon_pos + 1);
    
    // Handle special field mappings
    if (field == "site") {
        field = "domain";
    }
    
    return std::make_unique<Filter>(std::move(field), std::move(value));
}

std::string QueryParser::toRedisSyntax(const Node& node) {
    if (auto term = dynamic_cast<const Term*>(&node)) {
        if (term->value.empty()) {
            return "";
        }
        if (term->exact) {
            return "\"" + term->value + "\"";
        }
        return term->value;
    }
    
    if (auto and_node = dynamic_cast<const And*>(&node)) {
        if (and_node->children.empty()) {
            return "";
        }
        std::ostringstream oss;
        oss << "(";
        bool first = true;
        for (const auto& child : and_node->children) {
            auto child_syntax = toRedisSyntax(*child);
            if (!child_syntax.empty()) {
                if (!first) oss << " ";
                oss << child_syntax;
                first = false;
            }
        }
        oss << ")";
        return first ? "" : oss.str();
    }
    
    if (auto or_node = dynamic_cast<const Or*>(&node)) {
        if (or_node->children.empty()) {
            return "";
        }
        std::ostringstream oss;
        oss << "(";
        bool first = true;
        for (const auto& child : or_node->children) {
            auto child_syntax = toRedisSyntax(*child);
            if (!child_syntax.empty()) {
                if (!first) oss << " | ";
                oss << child_syntax;
                first = false;
            }
        }
        oss << ")";
        return first ? "" : oss.str();
    }
    
    if (auto filter = dynamic_cast<const Filter*>(&node)) {
        return "@" + filter->field + ":{" + filter->value + "}";
    }
    
    return "";
}

} // namespace hatef::search 