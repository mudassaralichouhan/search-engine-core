#pragma once
#include <string>
#include <memory>
#include <vector>
#include <string_view>

namespace hatef::search {

struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct Node {
    virtual ~Node() = default;
    // double-dispatch pretty-print to Redis syntax
    virtual std::string to_redis() const = 0;
};
using NodePtr = std::unique_ptr<Node>;

struct Term : Node {
    std::string value;
    bool exact{false};
    
    // Add explicit constructors
    Term() = default;
    Term(std::string val, bool ex = false) : value(std::move(val)), exact(ex) {}
    
    std::string to_redis() const override;
};

struct Filter : Node {
    std::string field;   // e.g. "domain"
    std::string value;   // e.g. "example.com"
    
    // Add explicit constructors
    Filter() = default;
    Filter(std::string f, std::string v) : field(std::move(f)), value(std::move(v)) {}
    
    std::string to_redis() const override;
};

struct And : Node { std::vector<NodePtr> children; std::string to_redis() const override; };
struct Or  : Node { std::vector<NodePtr> children; std::string to_redis() const override; };

class QueryParser {
public:
    /// Parse raw user query into AST. Throws ParseError on syntax issues.
    [[nodiscard]] NodePtr parse(std::string_view q) const;

    /// Convert AST node to Redis syntax (the method tests expect)
    [[nodiscard]] std::string toRedisSyntax(const Node& ast) const;
    
    /// Convenience: return full Redis syntax in one call.
    [[nodiscard]] std::string to_redis(std::string_view q) const;
};

} // namespace hatef::search 