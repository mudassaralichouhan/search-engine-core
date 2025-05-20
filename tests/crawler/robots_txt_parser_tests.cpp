#include <catch2/catch_test_macros.hpp>
#include "RobotsTxtParser.h"

TEST_CASE("RobotsTxtParser handles basic rules", "[RobotsTxtParser]") {
    RobotsTxtParser parser;
    
    SECTION("Parses simple disallow rules") {
        std::string content = R"(
            User-agent: *
            Disallow: /private/
            Disallow: /admin/
        )";
        
        parser.parseRobotsTxt("example.com", content);
        
        REQUIRE_FALSE(parser.isAllowed("https://example.com/private/page", "MyBot"));
        REQUIRE_FALSE(parser.isAllowed("https://example.com/admin/dashboard", "MyBot"));
        REQUIRE(parser.isAllowed("https://example.com/public/page", "MyBot"));
    }
    
    SECTION("Parses user-agent specific rules") {
        std::string content = R"(
            User-agent: MyBot
            Disallow: /mybot-private/
            
            User-agent: *
            Disallow: /private/
        )";
        
        parser.parseRobotsTxt("example.com", content);
        
        REQUIRE_FALSE(parser.isAllowed("https://example.com/mybot-private/page", "MyBot"));
        REQUIRE(parser.isAllowed("https://example.com/private/page", "MyBot"));
        REQUIRE_FALSE(parser.isAllowed("https://example.com/private/page", "OtherBot"));
    }
}

TEST_CASE("RobotsTxtParser handles allow rules", "[RobotsTxtParser]") {
    RobotsTxtParser parser;
    
    SECTION("Allow rules override disallow rules") {
        std::string content = R"(
            User-agent: *
            Disallow: /private/
            Allow: /private/public/
        )";
        
        parser.parseRobotsTxt("example.com", content);
        
        REQUIRE_FALSE(parser.isAllowed("https://example.com/private/secret", "MyBot"));
        REQUIRE(parser.isAllowed("https://example.com/private/public/page", "MyBot"));
    }
}

TEST_CASE("RobotsTxtParser handles crawl delay", "[RobotsTxtParser]") {
    RobotsTxtParser parser;
    
    SECTION("Parses crawl delay correctly") {
        std::string content = R"(
            User-agent: MyBot
            Crawl-delay: 2
            
            User-agent: *
            Crawl-delay: 1
        )";
        
        parser.parseRobotsTxt("example.com", content);
        
        REQUIRE(parser.getCrawlDelay("example.com", "MyBot").count() == 2000);
        REQUIRE(parser.getCrawlDelay("example.com", "OtherBot").count() == 1000);
    }
    
    SECTION("Uses default crawl delay when not specified") {
        std::string content = R"(
            User-agent: *
            Disallow: /private/
        )";
        
        parser.parseRobotsTxt("example.com", content);
        
        REQUIRE(parser.getCrawlDelay("example.com", "MyBot").count() == 1000); // Default 1 second
    }
}

TEST_CASE("RobotsTxtParser handles caching", "[RobotsTxtParser]") {
    RobotsTxtParser parser;
    
    SECTION("Caches robots.txt content") {
        std::string content = R"(
            User-agent: *
            Disallow: /private/
        )";
        
        parser.parseRobotsTxt("example.com", content);
        REQUIRE(parser.isCached("example.com"));
    }
    
    SECTION("Clears cache correctly") {
        std::string content = R"(
            User-agent: *
            Disallow: /private/
        )";
        
        parser.parseRobotsTxt("example.com", content);
        parser.clearCache("example.com");
        REQUIRE_FALSE(parser.isCached("example.com"));
    }
}

TEST_CASE("RobotsTxtParser handles pattern matching", "[RobotsTxtParser]") {
    RobotsTxtParser parser;
    
    SECTION("Matches wildcard patterns") {
        std::string content = R"(
            User-agent: *
            Disallow: /*.pdf$
            Disallow: /images/*.jpg$
        )";
        
        parser.parseRobotsTxt("example.com", content);
        
        REQUIRE_FALSE(parser.isAllowed("https://example.com/document.pdf", "MyBot"));
        REQUIRE_FALSE(parser.isAllowed("https://example.com/images/photo.jpg", "MyBot"));
        REQUIRE(parser.isAllowed("https://example.com/document.doc", "MyBot"));
        REQUIRE(parser.isAllowed("https://example.com/images/photo.png", "MyBot"));
    }
    
    SECTION("Handles empty robots.txt") {
        parser.parseRobotsTxt("example.com", "");
        REQUIRE(parser.isAllowed("https://example.com/any/path", "MyBot"));
    }
} 