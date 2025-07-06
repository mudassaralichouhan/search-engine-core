#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "../../include/search_engine/scoring/SearchScorer.h"

using namespace search_engine::scoring;
using Catch::Matchers::WithinRel;
using Catch::Matchers::WithinAbs;

class TestSearchScorer {
public:
    std::vector<std::unordered_map<std::string, std::string>> createTestDocuments() {
        return {
            {
                {"url", "doc1"},
                {"title", "Introduction to Machine Learning"},
                {"content", "Machine learning is a subset of artificial intelligence that focuses on algorithms."},
                {"description", "A comprehensive guide to ML"},
                {"domain", "example.com"},
                {"score", "1.0"}
            },
            {
                {"url", "doc2"},
                {"title", "Python Programming Tutorial"},
                {"content", "Python is a versatile programming language used for machine learning and data science."},
                {"description", "Learn Python from scratch"},
                {"domain", "github.com"},
                {"score", "0.9"}
            },
            {
                {"url", "doc3"},
                {"title", "Deep Learning with Neural Networks"},
                {"content", "Deep learning uses artificial neural networks to model complex patterns in data."},
                {"description", "Advanced ML techniques"},
                {"domain", "stackoverflow.com"},
                {"score", "0.8"}
            },
            {
                {"url", "doc4"},
                {"title", "Data Science Fundamentals"},
                {"content", "Data science combines statistics, programming, and domain knowledge."},
                {"description", "Essential data science concepts"},
                {"domain", "medium.com"},
                {"score", "0.7"}
            }
        };
    }
};

TEST_CASE_METHOD(TestSearchScorer, "Scoring Configuration", "[scoring][config]") {
    SECTION("Default configuration") {
        auto config = ScoringConfig::createDefault();
        
        REQUIRE(config.fieldWeights.title == 5.0);
        REQUIRE(config.fieldWeights.content == 1.0);
        REQUIRE(config.fieldWeights.description == 3.0);
        REQUIRE(config.fieldWeights.keywords == 4.0);
        
        REQUIRE(config.boostFactors.exactMatchBoost == 2.0);
        REQUIRE(config.boostFactors.titleMatchBoost == 1.5);
        
        REQUIRE(config.bm25Params.k1 == 1.2);
        REQUIRE(config.bm25Params.b == 0.75);
    }
    
    SECTION("Title-heavy configuration") {
        auto config = ScoringConfig::createTitleHeavy();
        
        REQUIRE(config.fieldWeights.title == 10.0);
        REQUIRE(config.fieldWeights.content == 0.5);
        REQUIRE(config.boostFactors.titleMatchBoost == 2.0);
    }
    
    SECTION("Content-heavy configuration") {
        auto config = ScoringConfig::createContentHeavy();
        
        REQUIRE(config.fieldWeights.title == 3.0);
        REQUIRE(config.fieldWeights.content == 2.0);
        REQUIRE(config.tfParams.maxTermFrequency == 20.0);
    }
}

TEST_CASE_METHOD(TestSearchScorer, "BM25 Scoring", "[scoring][bm25]") {
    auto scorer = SearchScorer::createBM25Scorer();
    auto docs = createTestDocuments();
    
    SECTION("Single term query") {
        auto scores = scorer->scoreResults(docs, "machine");
        
        REQUIRE(scores.size() >= 2);  // At least 2 documents should match
        
        // Document 1 should score higher (has "machine" in title)
        auto doc1Score = std::find_if(scores.begin(), scores.end(), 
            [](const auto& s) { return s.documentId == "doc1"; });
        auto doc2Score = std::find_if(scores.begin(), scores.end(), 
            [](const auto& s) { return s.documentId == "doc2"; });
        
        REQUIRE(doc1Score != scores.end());
        REQUIRE(doc2Score != scores.end());
        REQUIRE(doc1Score->totalScore > doc2Score->totalScore);
        
        // Check that title score is weighted higher
        REQUIRE(doc1Score->titleScore > 0);
        REQUIRE(doc1Score->matchedTerms >= 1);
    }
    
    SECTION("Multi-term query") {
        auto scores = scorer->scoreResults(docs, "machine learning");
        
        // Document 1 should score highest (has both terms in title)
        auto ranked = scorer->rankResults(docs, "machine learning", 2);
        
        REQUIRE(ranked.size() >= 1);
        REQUIRE(ranked[0].documentId == "doc1");
        REQUIRE(ranked[0].matchedTerms >= 2);
        REQUIRE(ranked[0].coverage > 0.5);  // At least 50% coverage
    }
    
    SECTION("Exact phrase matching") {
        auto scores = scorer->scoreResults(docs, "\"machine learning\"");
        
        auto doc1Score = std::find_if(scores.begin(), scores.end(), 
            [](const auto& s) { return s.documentId == "doc1"; });
        
        REQUIRE(doc1Score != scores.end());
        REQUIRE(doc1Score->exactMatchScore > 0);
    }
}

TEST_CASE_METHOD(TestSearchScorer, "TF-IDF Scoring", "[scoring][tfidf]") {
    auto scorer = SearchScorer::createTFIDFScorer();
    auto docs = createTestDocuments();
    
    SECTION("Term frequency calculation") {
        auto scores = scorer->scoreResults(docs, "learning");
        
        // All documents with "learning" should have positive scores
        for (const auto& score : scores) {
            if (score.documentId == "doc1" || score.documentId == "doc3") {
                REQUIRE(score.totalScore > 0);
            }
        }
    }
    
    SECTION("IDF weighting") {
        // "machine" appears in multiple docs, should have lower IDF than "neural"
        auto machineScores = scorer->scoreResults(docs, "machine");
        auto neuralScores = scorer->scoreResults(docs, "neural");
        
        // Document 3 has "neural" - a rarer term
        auto neuralDoc = std::find_if(neuralScores.begin(), neuralScores.end(),
            [](const auto& s) { return s.documentId == "doc3"; });
        
        if (neuralDoc != neuralScores.end()) {
            // The score for the rare term should be relatively high
            REQUIRE(neuralDoc->totalScore > 0);
        }
    }
}

TEST_CASE_METHOD(TestSearchScorer, "Combined RedisSearch Scoring", "[scoring][combined]") {
    auto scorer = SearchScorer::createRedisSearchScorer();
    auto docs = createTestDocuments();
    
    SECTION("Redis score integration") {
        auto scores = scorer->scoreResults(docs, "programming");
        
        // Document 2 has "programming" and high Redis score (0.9)
        auto doc2Score = std::find_if(scores.begin(), scores.end(),
            [](const auto& s) { return s.documentId == "doc2"; });
        
        REQUIRE(doc2Score != scores.end());
        REQUIRE(doc2Score->totalScore > 0);
        
        // The combined score should reflect both custom and Redis scores
        // Check that the explanation mentions combined scoring
        REQUIRE(doc2Score->explanation.find("Combined") != std::string::npos);
    }
}

TEST_CASE_METHOD(TestSearchScorer, "Ranking and Sorting", "[scoring][ranking]") {
    auto scorer = SearchScorer::createBM25Scorer();
    auto docs = createTestDocuments();
    
    SECTION("Top-K ranking") {
        auto top2 = scorer->rankResults(docs, "machine learning", 2);
        
        REQUIRE(top2.size() <= 2);
        
        // Results should be sorted by score (descending)
        for (size_t i = 1; i < top2.size(); ++i) {
            REQUIRE(top2[i-1].totalScore >= top2[i].totalScore);
        }
    }
    
    SECTION("All results ranking") {
        auto allRanked = scorer->rankResults(docs, "data", 0);  // 0 = return all
        
        // Should return all matching documents
        REQUIRE(allRanked.size() >= 1);
        
        // Check descending order
        for (size_t i = 1; i < allRanked.size(); ++i) {
            REQUIRE(allRanked[i-1].totalScore >= allRanked[i].totalScore);
        }
    }
}

TEST_CASE_METHOD(TestSearchScorer, "Field Weighting", "[scoring][weights]") {
    // Create custom config with extreme title weight
    ScoringConfig config;
    config.fieldWeights.title = 100.0;
    config.fieldWeights.content = 1.0;
    
    auto scorer = SearchScorer::createBM25Scorer(config);
    auto docs = createTestDocuments();
    
    SECTION("Title weight dominance") {
        auto scores = scorer->scoreResults(docs, "python");
        
        // Document 2 has "Python" in title, should score much higher
        auto ranked = scorer->rankResults(docs, "python", 2);
        
        REQUIRE(ranked.size() >= 1);
        REQUIRE(ranked[0].documentId == "doc2");
        REQUIRE(ranked[0].titleScore > ranked[0].contentScore * 50);  // Title heavily weighted
    }
}

TEST_CASE_METHOD(TestSearchScorer, "Boost Factors", "[scoring][boosts]") {
    ScoringConfig config;
    config.boostFactors.domainAuthorityBoost = 5.0;  // High boost for certain domains
    
    auto scorer = SearchScorer::createBM25Scorer(config);
    auto docs = createTestDocuments();
    
    SECTION("Domain authority boost") {
        auto scores = scorer->scoreResults(docs, "programming");
        
        // Document 2 (github.com) should get domain boost
        auto doc2Score = std::find_if(scores.begin(), scores.end(),
            [](const auto& s) { return s.documentId == "doc2"; });
        
        REQUIRE(doc2Score != scores.end());
        REQUIRE(doc2Score->boostScore > 0);
    }
}

TEST_CASE_METHOD(TestSearchScorer, "Score Normalization", "[scoring][normalization]") {
    ScoringConfig config;
    config.normalizeScores = true;
    
    auto scorer = SearchScorer::createBM25Scorer(config);
    auto docs = createTestDocuments();
    
    SECTION("Normalized score range") {
        auto scores = scorer->scoreResults(docs, "machine learning data");
        
        if (scores.size() > 1) {
            // Find min and max scores
            double minScore = scores[0].totalScore;
            double maxScore = scores[0].totalScore;
            
            for (const auto& score : scores) {
                minScore = std::min(minScore, score.totalScore);
                maxScore = std::max(maxScore, score.totalScore);
            }
            
            // Normalized scores should be in [0, 1] range
            REQUIRE(minScore >= 0.0);
            REQUIRE(maxScore <= 1.0);
        }
    }
}

TEST_CASE_METHOD(TestSearchScorer, "Utility Functions", "[scoring][utils]") {
    SECTION("Term extraction") {
        auto terms = scoring_utils::extractTerms("Machine Learning is AWESOME!");
        
        REQUIRE(terms.size() == 4);
        REQUIRE(std::find(terms.begin(), terms.end(), "machine") != terms.end());
        REQUIRE(std::find(terms.begin(), terms.end(), "learning") != terms.end());
        REQUIRE(std::find(terms.begin(), terms.end(), "is") != terms.end());
        REQUIRE(std::find(terms.begin(), terms.end(), "awesome") != terms.end());
    }
    
    SECTION("Term frequency calculation") {
        auto freqs = scoring_utils::calculateTermFrequencies("the quick brown fox jumps over the lazy dog");
        
        REQUIRE(freqs["the"] == 2);
        REQUIRE(freqs["quick"] == 1);
        REQUIRE(freqs["fox"] == 1);
    }
    
    SECTION("Jaccard similarity") {
        std::vector<std::string> terms1 = {"machine", "learning", "python"};
        std::vector<std::string> terms2 = {"machine", "learning", "java"};
        
        double similarity = scoring_utils::calculateJaccardSimilarity(terms1, terms2);
        
        // 2 common terms out of 4 unique terms = 0.5
        REQUIRE_THAT(similarity, WithinAbs(0.5, 0.01));
    }
    
    SECTION("Cosine similarity") {
        std::unordered_map<std::string, double> vec1 = {{"a", 1.0}, {"b", 2.0}};
        std::unordered_map<std::string, double> vec2 = {{"a", 2.0}, {"b", 1.0}};
        
        double similarity = scoring_utils::calculateCosineSimilarity(vec1, vec2);
        
        // Should be between 0 and 1
        REQUIRE(similarity >= 0.0);
        REQUIRE(similarity <= 1.0);
    }
}

TEST_CASE_METHOD(TestSearchScorer, "Edge Cases", "[scoring][edge_cases]") {
    auto scorer = SearchScorer::createBM25Scorer();
    
    SECTION("Empty query") {
        std::vector<std::unordered_map<std::string, std::string>> docs = {
            {{"url", "doc1"}, {"title", "Test"}, {"content", "Content"}}
        };
        
        auto scores = scorer->scoreResults(docs, "");
        
        REQUIRE((scores.empty() || scores[0].totalScore == 0));
    }
    
    SECTION("No matching documents") {
        auto docs = createTestDocuments();
        auto scores = scorer->scoreResults(docs, "nonexistentterm123456");
        
        // All scores should be 0 or documents filtered out
        for (const auto& score : scores) {
            REQUIRE(score.totalScore == 0);
        }
    }
    
    SECTION("Missing fields") {
        std::vector<std::unordered_map<std::string, std::string>> docs = {
            {{"url", "doc1"}, {"title", "Machine Learning"}},  // Missing content
            {{"url", "doc2"}, {"content", "Machine learning content"}}  // Missing title
        };
        
        auto scores = scorer->scoreResults(docs, "machine");
        
        // Should handle missing fields gracefully
        REQUIRE(scores.size() == 2);
        for (const auto& score : scores) {
            REQUIRE(score.totalScore >= 0);
        }
    }
} 