#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include "search_core/QueryParser.hpp"

using namespace hatef::search;

TEST_CASE("QueryParser basic queries", "[QueryParser][basic]") {
    QueryParser parser;
    
    SECTION("Simple word") {
        auto query = GENERATE("apple", "banana", "search");
        auto result = parser.to_redis(query);
        REQUIRE(result == query);
    }
    
    SECTION("Multiple words (implicit AND)") {
        auto result = parser.to_redis("apple banana");
        REQUIRE(result == "apple banana");
    }
    
    SECTION("Exact phrase") {
        auto result = parser.to_redis("\"apple pie\"");
        REQUIRE(result == "\"apple pie\"");
    }
    
    SECTION("Empty query throws") {
        REQUIRE_THROWS_AS(parser.to_redis(""), ParseError);
        REQUIRE_THROWS_AS(parser.to_redis("   "), ParseError);
        REQUIRE_THROWS_AS(parser.to_redis("\t\n"), ParseError);
    }
}

TEST_CASE("QueryParser boolean operators", "[QueryParser][operators]") {
    QueryParser parser;
    
    SECTION("Explicit AND") {
        auto result = parser.to_redis("apple AND banana");
        REQUIRE(result == "apple banana");
    }
    
    SECTION("OR operator") {
        auto query = GENERATE("apple OR banana", "apple or banana");
        auto result = parser.to_redis(query);
        REQUIRE(result == "apple|banana");
    }
    
    SECTION("Mixed operators") {
        auto result = parser.to_redis("apple AND banana OR cherry");
        REQUIRE(result == "apple banana|cherry");
    }
    
    SECTION("Multiple ORs") {
        auto result = parser.to_redis("apple OR banana OR cherry");
        REQUIRE(result == "apple|banana|cherry");
    }
}

TEST_CASE("QueryParser filters", "[QueryParser][filters]") {
    QueryParser parser;
    
    SECTION("Site filter") {
        auto result = parser.to_redis("site:example.com");
        REQUIRE(result == "@domain:{example.com}");
    }
    
    SECTION("Site filter with search") {
        auto result = parser.to_redis("site:example.com apple");
        REQUIRE(result == "@domain:{example.com} apple");
    }
    
    SECTION("Generic field filter") {
        auto field = GENERATE("title", "body", "author");
        auto query = std::string(field) + ":test";
        auto result = parser.to_redis(query);
        REQUIRE(result == "@" + std::string(field) + ":{test}");
    }
    
    SECTION("Multiple filters") {
        auto result = parser.to_redis("site:example.com title:launch");
        REQUIRE(result == "@domain:{example.com} @title:{launch}");
    }
}

TEST_CASE("QueryParser normalization", "[QueryParser][normalization]") {
    QueryParser parser;
    
    SECTION("Lowercase conversion") {
        auto result = parser.to_redis("ApPlE");
        REQUIRE(result == "apple");
    }
    
    SECTION("Punctuation stripping") {
        auto query = GENERATE("apple,", "apple!", "apple.", "apple;");
        auto result = parser.to_redis(query);
        REQUIRE(result == "apple");
    }
    
    SECTION("Preserve special characters") {
        // Quotes are preserved in exact phrases
        auto result1 = parser.to_redis("\"apple pie\"");
        REQUIRE(result1 == "\"apple pie\"");
        
        // Colons in filters
        auto result2 = parser.to_redis("site:test");
        REQUIRE(result2 == "@domain:{test}");
        
        // Hyphens preserved
        auto result3 = parser.to_redis("e-mail");
        REQUIRE(result3 == "e-mail");
    }
}

TEST_CASE("QueryParser unicode", "[QueryParser][unicode]") {
    QueryParser parser;
    
    SECTION("Unicode terms") {
        auto query = GENERATE("café", "résumé", "naïve");
        auto result = parser.to_redis(query);
        REQUIRE(!result.empty());
        // Should handle NFKC normalization
    }
}

TEST_CASE("QueryParser error handling", "[QueryParser][errors]") {
    QueryParser parser;
    
    SECTION("Unmatched quotes") {
        REQUIRE_THROWS_AS(parser.to_redis("\"unclosed"), ParseError);
        REQUIRE_THROWS_AS(parser.to_redis("unclosed\""), ParseError);
    }
    
    SECTION("Stray operators") {
        REQUIRE_THROWS_AS(parser.to_redis("OR"), ParseError);
        REQUIRE_THROWS_AS(parser.to_redis("AND"), ParseError);
        REQUIRE_THROWS_AS(parser.to_redis("apple OR"), ParseError);
        REQUIRE_THROWS_AS(parser.to_redis("AND apple"), ParseError);
    }
    
    SECTION("Invalid filters") {
        // Empty field
        auto result1 = parser.to_redis(":value");
        REQUIRE(result1 == "value");
        
        // Empty value
        auto result2 = parser.to_redis("field:");
        REQUIRE(result2 == "field");
    }
}

TEST_CASE("QueryParser complex queries", "[QueryParser][complex]") {
    QueryParser parser;
    
    SECTION("Combined features") {
        auto result = parser.to_redis("site:example.com \"apple pie\" OR banana recipe");
        // Should parse as: site:example.com AND "apple pie" OR banana AND recipe
        REQUIRE(result.find("@domain:{example.com}") != std::string::npos);
        REQUIRE(result.find("\"apple pie\"") != std::string::npos);
        REQUIRE(result.find("|") != std::string::npos);
    }
    
    SECTION("Multiple sites") {
        auto result = parser.to_redis("site:example.com site:test.org apple");
        REQUIRE(result == "@domain:{example.com} @domain:{test.org} apple");
    }
} 