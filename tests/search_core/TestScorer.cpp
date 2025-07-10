#include <catch2/catch_test_macros.hpp>
#include "search_core/Scorer.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include "../../src/common/JsMinifier.h"
#include <cassert>

using namespace hatef::search;

void test_js_minifier() {
    JsMinifier& minifier = JsMinifier::getInstance();
    minifier.setEnabled(true);
    std::string js = R"(
        // This is a comment
        var x = 1; /* multi-line
        comment */ var y = 2;   
        function foo() {   return x + y;   } // end of line comment
    )";
    std::string expected = "var x=1;var y=2;function foo(){return x+y;}";
    std::string result = minifier.process(js);
    assert(result == expected && "JsMinifier minify failed");
    std::cout << "JsMinifier minify test passed!\n";
}

int main() {
    test_js_minifier();
    return 0;
}

TEST_CASE("Scorer default configuration", "[Scorer]") {
    Scorer scorer;
    
    SECTION("Default field weights") {
        const auto& config = scorer.getConfig();
        REQUIRE(config.field_weights.at("title") == 2.0);
        REQUIRE(config.field_weights.at("body") == 1.0);
        REQUIRE(config.offset_boost == 0.1);
    }
    
    SECTION("Build default arguments") {
        auto args = scorer.buildArgs();
        
        // Log the arguments
        std::cout << "[TEST] Scorer args count: " << args.size() << std::endl;
        for (size_t i = 0; i < args.size(); ++i) {
            std::cout << "[TEST]   Arg[" << i << "]: " << args[i] << std::endl;
        }
        
        REQUIRE(!args.empty());
        
        // Check for required elements
        bool has_scorer = std::find(args.begin(), args.end(), "SCORER") != args.end();
        bool has_tfidf = std::find(args.begin(), args.end(), "TFIDF") != args.end();
        bool has_weights = std::find(args.begin(), args.end(), "WEIGHTS") != args.end();
        
        std::cout << "[TEST] has_scorer: " << has_scorer << std::endl;
        std::cout << "[TEST] has_tfidf: " << has_tfidf << std::endl;
        std::cout << "[TEST] has_weights: " << has_weights << std::endl;
        
        REQUIRE(has_scorer);
        REQUIRE(has_tfidf);
        
        // Note: WEIGHTS is not a valid argument for FT.SEARCH
        // Field weights should be specified during index creation with FT.CREATE
        // So we should NOT expect WEIGHTS in the search arguments
        REQUIRE_FALSE(has_weights); // Changed from REQUIRE(has_weights)
    }
}

TEST_CASE("Scorer configuration loading", "[Scorer]") {
    // Create a temporary config file
    const std::string temp_config = "test_scoring.json";
    
    SECTION("Load valid configuration") {
        // Create test config
        std::ofstream file(temp_config);
        file << R"({
            "field_weights": {
                "title": 3.0,
                "body": 1.5,
                "meta": 0.5
            },
            "offset_boost": 0.2
        })";
        file.close();
        
        Scorer scorer(temp_config);
        const auto& config = scorer.getConfig();
        
        REQUIRE(config.field_weights.at("title") == 3.0);
        REQUIRE(config.field_weights.at("body") == 1.5);
        REQUIRE(config.field_weights.at("meta") == 0.5);
        REQUIRE(config.offset_boost == 0.2);
        
        // Clean up
        std::filesystem::remove(temp_config);
    }
    
    SECTION("Load configuration with missing fields") {
        // Create test config with missing offset_boost
        std::ofstream file(temp_config);
        file << R"({
            "field_weights": {
                "title": 4.0
            }
        })";
        file.close();
        
        Scorer scorer(temp_config);
        const auto& config = scorer.getConfig();
        
        REQUIRE(config.field_weights.at("title") == 4.0);
        REQUIRE(config.offset_boost == 0.1); // Should use default
        
        // Clean up
        std::filesystem::remove(temp_config);
    }
    
    SECTION("Invalid configuration file") {
        // Should fall back to default config
        Scorer scorer("nonexistent_config.json");
        const auto& config = scorer.getConfig();
        
        REQUIRE(config.field_weights.at("title") == 2.0);
        REQUIRE(config.field_weights.at("body") == 1.0);
        REQUIRE(config.offset_boost == 0.1);
    }
}

TEST_CASE("Scorer argument building", "[Scorer]") {
    SECTION("Arguments with custom weights") {
        const std::string temp_config = "test_custom_scoring.json";
        
        // Create test config
        std::ofstream file(temp_config);
        file << R"({
            "field_weights": {
                "title": 2.5,
                "body": 1.0,
                "tags": 1.5
            },
            "offset_boost": 0.15
        })";
        file.close();
        
        Scorer scorer(temp_config);
        auto args = scorer.buildArgs();
        
        // Log the arguments
        std::cout << "[TEST] Custom scorer args count: " << args.size() << std::endl;
        for (size_t i = 0; i < args.size(); ++i) {
            std::cout << "[TEST]   Arg[" << i << "]: " << args[i] << std::endl;
        }
        
        // Verify structure
        REQUIRE(!args.empty());
        
        // Should only have SCORER and TFIDF
        REQUIRE(args.size() == 2);
        REQUIRE(args[0] == "SCORER");
        REQUIRE(args[1] == "TFIDF");
        
        // Note: Field weights from config are intended for index creation (FT.CREATE),
        // not for search queries (FT.SEARCH)
        
        // Clean up
        std::filesystem::remove(temp_config);
    }
    
    SECTION("Arguments format validation") {
        Scorer scorer;
        auto args = scorer.buildArgs();
        
        // Should only have SCORER and algorithm name
        REQUIRE(args.size() == 2);
        
        // First two should be SCORER and algorithm
        REQUIRE(args[0] == "SCORER");
        REQUIRE(args[1] == "TFIDF");
        
        // Should NOT contain WEIGHTS or PARAMS (not valid for FT.SEARCH)
        bool has_weights = std::find(args.begin(), args.end(), "WEIGHTS") != args.end();
        REQUIRE_FALSE(has_weights);
        
        bool has_params = std::find(args.begin(), args.end(), "PARAMS") != args.end();
        REQUIRE_FALSE(has_params);
    }
}

TEST_CASE("Scorer configuration reload", "[Scorer]") {
    const std::string temp_config = "test_reload_scoring.json";
    
    // Create initial config
    std::ofstream file(temp_config);
    file << R"({
        "field_weights": {
            "title": 1.0
        },
        "offset_boost": 0.1
    })";
    file.close();
    
    Scorer scorer(temp_config);
    REQUIRE(scorer.getConfig().field_weights.at("title") == 1.0);
    
    // Update config file
    file.open(temp_config);
    file << R"({
        "field_weights": {
            "title": 5.0
        },
        "offset_boost": 0.3
    })";
    file.close();
    
    // Reload
    scorer.reload(temp_config);
    REQUIRE(scorer.getConfig().field_weights.at("title") == 5.0);
    REQUIRE(scorer.getConfig().offset_boost == 0.3);
    
    // Clean up
    std::filesystem::remove(temp_config);
} 