#include <catch2/catch_test_macros.hpp>
#include "ContentParser.h"

TEST_CASE("ContentParser extracts title correctly", "[ContentParser]") {
    ContentParser parser;
    
    SECTION("Extracts title from valid HTML") {
        std::string html = "<html><head><title>Test Title</title></head><body></body></html>";
        auto title = parser.extractTitle(html);
        REQUIRE(title.has_value());
        REQUIRE(title.value() == "Test Title");
    }
    
    SECTION("Returns empty optional for HTML without title") {
        std::string html = "<html><head></head><body></body></html>";
        auto title = parser.extractTitle(html);
        REQUIRE_FALSE(title.has_value());
    }
}

TEST_CASE("ContentParser extracts meta description correctly", "[ContentParser]") {
    ContentParser parser;
    
    SECTION("Extracts meta description from valid HTML") {
        std::string html = "<html><head><meta name=\"description\" content=\"Test Description\"></head><body></body></html>";
        auto description = parser.extractMetaDescription(html);
        REQUIRE(description.has_value());
        REQUIRE(description.value() == "Test Description");
    }
    
    SECTION("Returns empty optional for HTML without meta description") {
        std::string html = "<html><head></head><body></body></html>";
        auto description = parser.extractMetaDescription(html);
        REQUIRE_FALSE(description.has_value());
    }
}

TEST_CASE("ContentParser extracts text content correctly", "[ContentParser]") {
    ContentParser parser;
    
    SECTION("Extracts text from simple HTML") {
        std::string html = "<html><body><p>Test paragraph</p></body></html>";
        std::string text = parser.extractText(html);
        REQUIRE(text.find("Test paragraph") != std::string::npos);
    }
    
    SECTION("Ignores script and style content") {
        std::string html = "<html><body><script>var x = 1;</script><p>Test paragraph</p><style>body { color: red; }</style></body></html>";
        std::string text = parser.extractText(html);
        REQUIRE(text.find("var x = 1") == std::string::npos);
        REQUIRE(text.find("color: red") == std::string::npos);
        REQUIRE(text.find("Test paragraph") != std::string::npos);
    }
}

TEST_CASE("ContentParser extracts and normalizes links correctly", "[ContentParser]") {
    ContentParser parser;
    
    SECTION("Extracts absolute URLs") {
        std::string html = "<html><body><a href=\"https://example.com\">Link</a></body></html>";
        std::string baseUrl = "https://base.com";
        auto links = parser.extractLinks(html, baseUrl);
        REQUIRE(links.size() == 1);
        REQUIRE(links[0] == "https://example.com");
    }
    
    SECTION("Normalizes relative URLs") {
        std::string html = "<html><body><a href=\"/path\">Link</a></body></html>";
        std::string baseUrl = "https://base.com";
        auto links = parser.extractLinks(html, baseUrl);
        REQUIRE(links.size() == 1);
        REQUIRE(links[0] == "https://base.com/path");
    }
    
    SECTION("Normalizes protocol-relative URLs") {
        std::string html = "<html><body><a href=\"//example.com\">Link</a></body></html>";
        std::string baseUrl = "https://base.com";
        auto links = parser.extractLinks(html, baseUrl);
        REQUIRE(links.size() == 1);
        REQUIRE(links[0] == "https://example.com");
    }
}

TEST_CASE("ContentParser validates URLs correctly", "[ContentParser]") {
    ContentParser parser;
    
    SECTION("Validates correct URLs") {
        REQUIRE(parser.isValidUrl("https://example.com"));
        REQUIRE(parser.isValidUrl("http://sub.example.com/path"));
        REQUIRE(parser.isValidUrl("https://example.com:8080/path?query=value"));
        REQUIRE(parser.isValidUrl("https://fa.wikipedia.org/wiki/%D8%A7%DB%8C%D8%B1%D8%A7%D9%86"));
        REQUIRE(parser.isValidUrl("https://fa.wikipedia.org/wiki/ایران"));
        REQUIRE(parser.isValidUrl("https://fa.فرش.ایران/wiki/ایران"));
    }
    
    SECTION("Rejects invalid URLs") {
        REQUIRE_FALSE(parser.isValidUrl("not-a-url"));
        REQUIRE_FALSE(parser.isValidUrl("ftp://example.com")); // Only http/https supported
        REQUIRE_FALSE(parser.isValidUrl("https://"));
    }
}

TEST_CASE("ContentParser parses complete HTML document", "[ContentParser]") {
    ContentParser parser;
    
    SECTION("Parses all components correctly") {
        std::string html = R"(
            <html>
                <head>
                    <title>Test Title</title>
                    <meta name="description" content="Test Description">
                </head>
                <body>
                    <h1>Main Heading</h1>
                    <p>Test paragraph</p>
                    <a href="https://example.com">Link</a>
                </body>
            </html>
        )";
        
        ParsedContent result = parser.parse(html, "https://base.com");
        
        REQUIRE(result.title == "Test Title");
        REQUIRE(result.metaDescription == "Test Description");
        REQUIRE(result.textContent.find("Main Heading") != std::string::npos);
        REQUIRE(result.textContent.find("Test paragraph") != std::string::npos);
        REQUIRE(result.links.size() == 1);
        REQUIRE(result.links[0] == "https://example.com");
    }
} 