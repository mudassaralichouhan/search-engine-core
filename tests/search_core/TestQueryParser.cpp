#include <catch2/catch_test_macros.hpp>
#include "search_core/QueryParser.hpp"

using namespace hatef::search;

TEST_CASE("QueryParser empty and simple queries", "[QueryParser]") {
    QueryParser parser;
    
    SECTION("Empty query") {
        auto ast = parser.parse("");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        REQUIRE(result == "");
    }
    
    SECTION("Single term") {
        auto ast = parser.parse("hello");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        REQUIRE(result == "hello");
    }
    
    SECTION("Multiple terms (implicit AND)") {
        auto ast = parser.parse("hello world");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        REQUIRE(result == "(hello world)");
    }
}

TEST_CASE("QueryParser phrase queries", "[QueryParser]") {
    QueryParser parser;
    
    SECTION("Exact phrase") {
        auto ast = parser.parse("\"quick brown\"");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        REQUIRE(result == "\"quick brown\"");
    }
    
    SECTION("Phrase with other terms") {
        auto ast = parser.parse("\"quick brown\" fox");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        REQUIRE(result.find("\"quick brown\"") != std::string::npos);
        REQUIRE(result.find("fox") != std::string::npos);
    }
}

TEST_CASE("QueryParser boolean operators", "[QueryParser]") {
    QueryParser parser;
    
    SECTION("Explicit AND") {
        auto ast = parser.parse("hello AND world");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        REQUIRE(result == "(hello world)");
    }
    
    SECTION("OR operator") {
        auto ast = parser.parse("hello OR world");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        REQUIRE(result == "(hello | world)");
    }
    
    SECTION("Mixed operators") {
        auto ast = parser.parse("quick OR brown AND fox");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        // Should handle precedence correctly
        REQUIRE(!result.empty());
    }
}

TEST_CASE("QueryParser filters", "[QueryParser]") {
    QueryParser parser;
    
    SECTION("Site filter") {
        auto ast = parser.parse("site:example.com");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        REQUIRE(result == "@domain:{example.com}");
    }
    
    SECTION("Generic filter") {
        auto ast = parser.parse("author:john");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        REQUIRE(result == "@author:{john}");
    }
    
    SECTION("Filter with search terms") {
        auto ast = parser.parse("site:example.com hello world");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        REQUIRE(result.find("@domain:{example.com}") != std::string::npos);
        REQUIRE(result.find("hello") != std::string::npos);
        REQUIRE(result.find("world") != std::string::npos);
    }
}

TEST_CASE("QueryParser normalization", "[QueryParser]") {
    QueryParser parser;
    
    SECTION("Case normalization") {
        auto ast1 = parser.parse("Hello World");
        auto ast2 = parser.parse("hello world");
        REQUIRE(ast1 != nullptr);
        REQUIRE(ast2 != nullptr);
        auto result1 = parser.toRedisSyntax(*ast1);
        auto result2 = parser.toRedisSyntax(*ast2);
        REQUIRE(result1 == result2);
    }
    
    SECTION("Punctuation handling") {
        auto ast = parser.parse("hello, world!");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        REQUIRE(result == "(hello world)");
    }
    
    SECTION("Preserve quotes and colons") {
        auto ast = parser.parse("\"hello world\" site:test");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        REQUIRE(result.find("\"hello world\"") != std::string::npos);
        REQUIRE(result.find("@domain:{test}") != std::string::npos);
    }
}

TEST_CASE("QueryParser Unicode support", "[QueryParser]") {
    QueryParser parser;
    
    SECTION("Unicode characters") {
        auto ast = parser.parse("café résumé");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        REQUIRE(!result.empty());
    }
}

TEST_CASE("QueryParser edge cases", "[QueryParser]") {
    QueryParser parser;
    
    SECTION("Malformed filter") {
        auto ast = parser.parse("site:");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        REQUIRE(result == "site");
    }
    
    SECTION("Only operators") {
        auto ast = parser.parse("AND OR");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        // Should handle gracefully
        REQUIRE(!result.empty());
    }
    
    SECTION("Unclosed quotes") {
        auto ast = parser.parse("\"unclosed quote");
        REQUIRE(ast != nullptr);
        auto result = parser.toRedisSyntax(*ast);
        // Should handle gracefully
        REQUIRE(!result.empty());
    }
} 