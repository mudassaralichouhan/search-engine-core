#include "search_core/QueryParser.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <ranges>
#include <span>

namespace hatef::search {

namespace {

// Token types
enum class TokenType {
    WORD,
    QUOTED_STRING,
    AND_OP,
    OR_OP,
    FILTER,
    EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string value;
    std::string field; // For filter tokens
};

class Lexer {
public:
    explicit Lexer(std::string_view input) : input_(input), pos_(0) {}
    
    Token next() {
        skipWhitespace();
        
        if (pos_ >= input_.size()) {
            return {TokenType::EOF_TOKEN, "", ""};
        }
        
        // Check for quoted string
        if (input_[pos_] == '"') {
            return parseQuotedString();
        }
        
        // Parse word/operator/filter
        auto word = parseWord();
        
        if (word.empty()) {
            throw ParseError("Unexpected character at position " + std::to_string(pos_));
        }
        
        // Check for filter syntax (field:value)
        if (auto colonPos = word.find(':'); colonPos != std::string::npos) {
            // Handle edge cases
            if (colonPos == 0) {
                // ":value" -> treat as "value"
                std::string value = word.substr(1);
                return {TokenType::WORD, normalize(value), ""};
            } else if (colonPos == word.size() - 1) {
                // "field:" -> treat as "field"
                std::string field = word.substr(0, colonPos);
                return {TokenType::WORD, normalize(field), ""};
            } else {
                // Normal filter case
                std::string field = word.substr(0, colonPos);
                std::string value = word.substr(colonPos + 1);
                
                // Handle site: alias
                if (field == "site") {
                    field = "domain";
                }
                
                return {TokenType::FILTER, value, field};
            }
        }
        
        // Check for operators
        std::string wordLower = word;
        std::transform(wordLower.begin(), wordLower.end(), wordLower.begin(), ::tolower);
        
        if (wordLower == "and") {
            return {TokenType::AND_OP, "AND", ""};
        }
        if (wordLower == "or" || wordLower == "|") {
            return {TokenType::OR_OP, "OR", ""};
        }
        
        return {TokenType::WORD, normalize(word), ""};
    }
    
    Token peek() {
        auto savedPos = pos_;
        auto token = next();
        pos_ = savedPos;
        return token;
    }

private:
    void skipWhitespace() {
        while (pos_ < input_.size() && std::isspace(input_[pos_])) {
            ++pos_;
        }
    }
    
    Token parseQuotedString() {
        ++pos_; // Skip opening quote
        std::string value;
        
        while (pos_ < input_.size() && input_[pos_] != '"') {
            value += input_[pos_++];
        }
        
        if (pos_ >= input_.size()) {
            throw ParseError("Unmatched quote in query");
        }
        
        ++pos_; // Skip closing quote
        return {TokenType::QUOTED_STRING, value, ""};
    }
    
    std::string parseWord() {
        std::string word;
        
        while (pos_ < input_.size() && !std::isspace(input_[pos_]) && input_[pos_] != '"') {
            word += input_[pos_++];
        }
        
        return word;
    }
    
    std::string normalize(const std::string& text) {
        std::string result;
        
        for (char c : text) {
            if (std::isalnum(c) || c == '-' || c == '|' || c == ':') {
                result += std::tolower(c);
            }
        }
        
        return result;
    }
    
    std::string_view input_;
    size_t pos_;
};

class Parser {
public:
    explicit Parser(std::string_view input) : lexer_(input) {}
    
    NodePtr parse() {
        auto result = parseExpression();
        
        auto token = lexer_.next();
        if (token.type != TokenType::EOF_TOKEN) {
            throw ParseError("Unexpected token: " + token.value);
        }
        
        if (!result) {
            throw ParseError("Empty query");
        }
        
        return result;
    }

private:
    NodePtr parseExpression() {
        auto left = parseTerm();
        
        if (!left) {
            return nullptr;
        }
        
        while (true) {
            auto token = lexer_.peek();
            
            if (token.type == TokenType::OR_OP) {
                lexer_.next(); // Consume OR
                auto right = parseTerm();
                
                if (!right) {
                    throw ParseError("Expected term after OR");
                }
                
                if (auto* orNode = dynamic_cast<Or*>(left.get())) {
                    orNode->children.push_back(std::move(right));
                } else {
                    auto newOr = std::make_unique<Or>();
                    newOr->children.push_back(std::move(left));
                    newOr->children.push_back(std::move(right));
                    left = std::move(newOr);
                }
            } else if (token.type == TokenType::AND_OP) {
                lexer_.next(); // Consume AND
                auto right = parseTerm();
                
                if (!right) {
                    throw ParseError("Expected term after AND");
                }
                
                if (auto* andNode = dynamic_cast<And*>(left.get())) {
                    andNode->children.push_back(std::move(right));
                } else {
                    auto newAnd = std::make_unique<And>();
                    newAnd->children.push_back(std::move(left));
                    newAnd->children.push_back(std::move(right));
                    left = std::move(newAnd);
                }
            } else if (token.type == TokenType::WORD || token.type == TokenType::QUOTED_STRING || token.type == TokenType::FILTER) {
                // Implicit AND
                auto right = parseTerm();
                
                if (!right) {
                    break;
                }
                
                if (auto* andNode = dynamic_cast<And*>(left.get())) {
                    andNode->children.push_back(std::move(right));
                } else {
                    auto newAnd = std::make_unique<And>();
                    newAnd->children.push_back(std::move(left));
                    newAnd->children.push_back(std::move(right));
                    left = std::move(newAnd);
                }
            } else {
                break;
            }
        }
        
        return left;
    }
    
    NodePtr parseTerm() {
        auto token = lexer_.peek();
        
        switch (token.type) {
            case TokenType::WORD:
                lexer_.next();
                return std::make_unique<Term>(Term{token.value, false});
                
            case TokenType::QUOTED_STRING:
                lexer_.next();
                return std::make_unique<Term>(Term{token.value, true});
                
            case TokenType::FILTER:
                lexer_.next();
                return std::make_unique<Filter>(Filter{token.field, token.value});
                
            default:
                return nullptr;
        }
    }
    
    Lexer lexer_;
};

} // anonymous namespace

// Node implementations
std::string Term::to_redis() const {
    if (exact) {
        return "\"" + value + "\"";
    }
    return value;
}

std::string Filter::to_redis() const {
    return "@" + field + ":{" + value + "}";
}

std::string And::to_redis() const {
    std::ostringstream oss;
    for (size_t i = 0; i < children.size(); ++i) {
        if (i > 0) oss << " ";
        oss << children[i]->to_redis();
    }
    return oss.str();
}

std::string Or::to_redis() const {
    std::ostringstream oss;
    for (size_t i = 0; i < children.size(); ++i) {
        if (i > 0) oss << "|";
        oss << children[i]->to_redis();
    }
    return oss.str();
}

// QueryParser implementation
NodePtr QueryParser::parse(std::string_view q) const {
    // Trim whitespace
    auto start = q.find_first_not_of(" \t\n\r");
    auto end = q.find_last_not_of(" \t\n\r");
    
    if (start == std::string_view::npos) {
        throw ParseError("Empty query");
    }
    
    q = q.substr(start, end - start + 1);
    
    Parser parser(q);
    return parser.parse();
}

std::string QueryParser::toRedisSyntax(const Node& ast) const {
    return ast.to_redis();
}

std::string QueryParser::to_redis(std::string_view q) const {
    auto ast = parse(q);
    return ast->to_redis();
}

} // namespace hatef::search 