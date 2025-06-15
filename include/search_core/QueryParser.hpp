#pragma once

#include <memory>
#include <string>
#include <vector>

namespace hatef::search {

// AST Node hierarchy
struct Node { 
    virtual ~Node() = default; 
};

struct Term : Node { 
    std::string value; 
    bool exact = false;
    
    Term(std::string val, bool is_exact = false) 
        : value(std::move(val)), exact(is_exact) {}
};

struct And : Node { 
    std::vector<std::unique_ptr<Node>> children; 
    
    void addChild(std::unique_ptr<Node> child) {
        children.push_back(std::move(child));
    }
};

struct Or : Node { 
    std::vector<std::unique_ptr<Node>> children; 
    
    void addChild(std::unique_ptr<Node> child) {
        children.push_back(std::move(child));
    }
};

struct Filter : Node { 
    std::string field;
    std::string value; 
    
    Filter(std::string f, std::string v) 
        : field(std::move(f)), value(std::move(v)) {}
};

class QueryParser {
public:
    /// Parse a user query string into an AST
    std::unique_ptr<Node> parse(const std::string& query);
    
    /// Convert an AST node to RedisSearch syntax
    std::string toRedisSyntax(const Node& node);

private:
    /// Normalize text (lowercase, NFKC, strip punctuation)
    std::string normalize(const std::string& text);
    
    /// Tokenize the normalized query
    std::vector<std::string> tokenize(const std::string& text);
    
    /// Parse tokens into AST
    std::unique_ptr<Node> parseTokens(const std::vector<std::string>& tokens);
    
    /// Handle special tokens like site:, AND, OR
    bool isSpecialToken(const std::string& token);
    
    /// Convert filter token to Filter node
    std::unique_ptr<Filter> parseFilter(const std::string& token);
};

} // namespace hatef::search 