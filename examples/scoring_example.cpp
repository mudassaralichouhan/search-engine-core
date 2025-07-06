#include "../include/search_engine/scoring/SearchScorer.h"
#include "../include/search_engine/storage/RedisSearchStorage.h"
#include "../include/Logger.h"
#include <iostream>
#include <iomanip>

using namespace search_engine;
using namespace search_engine::scoring;
using namespace search_engine::storage;

void demonstrateScoring() {
    std::cout << "\n=== Search Result Scoring and Ranking Demo ===\n\n";
    
    // 1. Create different scoring configurations
    std::cout << "1. Scoring Configurations:\n";
    
    auto defaultConfig = ScoringConfig::createDefault();
    std::cout << "   Default: Title weight = " << defaultConfig.fieldWeights.title 
              << ", Content weight = " << defaultConfig.fieldWeights.content << "\n";
    
    auto titleHeavyConfig = ScoringConfig::createTitleHeavy();
    std::cout << "   Title-Heavy: Title weight = " << titleHeavyConfig.fieldWeights.title 
              << ", Content weight = " << titleHeavyConfig.fieldWeights.content << "\n";
    
    // 2. Create scorer with different algorithms
    std::cout << "\n2. Creating Scorers with Different Algorithms:\n";
    
    auto bm25Scorer = SearchScorer::createBM25Scorer();
    std::cout << "   - " << bm25Scorer->getAlgorithmName() << " scorer created\n";
    
    auto tfidfScorer = SearchScorer::createTFIDFScorer();
    std::cout << "   - " << tfidfScorer->getAlgorithmName() << " scorer created\n";
    
    auto combinedScorer = SearchScorer::createRedisSearchScorer();
    std::cout << "   - " << combinedScorer->getAlgorithmName() << " scorer created\n";
    
    // 3. Simulate RedisSearch results
    std::cout << "\n3. Simulating RedisSearch Results:\n";
    
    std::vector<std::unordered_map<std::string, std::string>> redisResults = {
        {
            {"url", "https://example.com/machine-learning-intro"},
            {"title", "Introduction to Machine Learning"},
            {"content", "Machine learning is a subset of artificial intelligence that enables systems to learn and improve from experience. This comprehensive guide covers supervised learning, unsupervised learning, and reinforcement learning techniques."},
            {"description", "A beginner's guide to machine learning concepts and algorithms"},
            {"keywords", "machine learning|AI|algorithms|tutorial"},
            {"domain", "example.com"},
            {"score", "0.8"}  // RedisSearch base score
        },
        {
            {"url", "https://github.com/awesome-ml/resources"},
            {"title", "Awesome Machine Learning Resources"},
            {"content", "A curated list of machine learning frameworks, libraries, and software. Includes Python libraries like scikit-learn, TensorFlow, and PyTorch."},
            {"description", "Curated list of ML resources"},
            {"keywords", "machine learning|python|resources|github"},
            {"domain", "github.com"},
            {"score", "0.75"}
        },
        {
            {"url", "https://blog.tech/deep-learning-tutorial"},
            {"title", "Deep Learning Tutorial for Beginners"},
            {"content", "Deep learning is a subset of machine learning that uses neural networks. This tutorial covers the basics of neural networks, backpropagation, and common architectures."},
            {"description", "Learn deep learning from scratch"},
            {"keywords", "deep learning|neural networks|tutorial"},
            {"domain", "blog.tech"},
            {"score", "0.7"}
        },
        {
            {"url", "https://docs.python.org/ml-guide"},
            {"title", "Python Machine Learning Guide"},
            {"content", "Python has become the de facto language for machine learning. This guide shows how to use Python for various ML tasks including data preprocessing, model training, and evaluation."},
            {"description", "Official Python ML documentation"},
            {"keywords", "python|machine learning|programming|guide"},
            {"domain", "python.org"},
            {"score", "0.85"}
        }
    };
    
    std::cout << "   Loaded " << redisResults.size() << " documents from RedisSearch\n";
    
    // 4. Score and rank with different queries
    std::cout << "\n4. Scoring Results for Different Queries:\n";
    
    std::vector<std::string> queries = {
        "machine learning",
        "\"machine learning\" python",
        "deep learning tutorial"
    };
    
    for (const auto& query : queries) {
        std::cout << "\n   Query: \"" << query << "\"\n";
        std::cout << "   " << std::string(50, '-') << "\n";
        
        // Score with BM25
        auto bm25Scores = bm25Scorer->rankResults(redisResults, query, 3);
        
        std::cout << "   BM25 Ranking:\n";
        for (size_t i = 0; i < bm25Scores.size(); ++i) {
            const auto& score = bm25Scores[i];
            std::cout << "   " << (i + 1) << ". " << std::setw(40) << std::left;
            
            // Find the title for this document
            std::string title = "Unknown";
            for (const auto& doc : redisResults) {
                if (doc.count("url") && doc.at("url") == score.documentId) {
                    title = doc.at("title");
                    break;
                }
            }
            
            std::cout << title.substr(0, 40) << " | Score: " << std::fixed << std::setprecision(3) << score.totalScore;
            std::cout << " (T:" << score.titleScore << " C:" << score.contentScore << " M:" << score.matchedTerms << "/" << score.totalTerms << ")\n";
        }
        
        // Score with Combined algorithm
        auto combinedScores = combinedScorer->rankResults(redisResults, query, 3);
        
        std::cout << "\n   RedisSearch+BM25 Combined Ranking:\n";
        for (size_t i = 0; i < combinedScores.size(); ++i) {
            const auto& score = combinedScores[i];
            std::cout << "   " << (i + 1) << ". " << std::setw(40) << std::left;
            
            std::string title = "Unknown";
            for (const auto& doc : redisResults) {
                if (doc.count("url") && doc.at("url") == score.documentId) {
                    title = doc.at("title");
                    break;
                }
            }
            
            std::cout << title.substr(0, 40) << " | Score: " << std::fixed << std::setprecision(3) << score.totalScore << "\n";
        }
    }
    
    // 5. Demonstrate custom scoring configuration
    std::cout << "\n5. Custom Scoring Configuration:\n";
    
    ScoringConfig customConfig;
    customConfig.fieldWeights.title = 10.0;     // Heavily favor title matches
    customConfig.fieldWeights.content = 0.5;    // Reduce content weight
    customConfig.boostFactors.exactMatchBoost = 3.0;  // Triple exact match boost
    customConfig.boostFactors.domainAuthorityBoost = 2.0;  // Double domain boost for github.com
    
    auto customScorer = SearchScorer::createBM25Scorer(customConfig);
    
    std::cout << "   Custom config: Title weight = 10.0, Exact match boost = 3.0\n";
    
    auto customScores = customScorer->rankResults(redisResults, "machine learning", 4);
    
    std::cout << "\n   Custom Scoring Results:\n";
    for (const auto& score : customScores) {
        std::cout << "   - Doc: " << score.documentId << "\n";
        std::cout << "     Total Score: " << score.totalScore << "\n";
        std::cout << "     Breakdown: Title=" << score.titleScore 
                  << ", Content=" << score.contentScore
                  << ", Exact=" << score.exactMatchScore
                  << ", Boost=" << score.boostScore << "\n";
        if (!score.explanation.empty()) {
            std::cout << "     Explanation: " << score.explanation << "\n";
        }
    }
    
    // 6. Performance comparison
    std::cout << "\n6. Performance Metrics:\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000; ++i) {
        bm25Scorer->scoreResults(redisResults, "machine learning");
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "   BM25: 1000 scoring operations in " << duration.count() << " microseconds\n";
    std::cout << "   Average: " << (duration.count() / 1000.0) << " microseconds per operation\n";
}

#ifdef REDIS_AVAILABLE
void demonstrateRedisIntegration() {
    std::cout << "\n\n=== RedisSearch Integration Demo ===\n\n";
    
    try {
        // Connect to Redis
        auto storage = std::make_unique<RedisSearchStorage>(
            "tcp://127.0.0.1:6379",
            "scoring_demo_index",
            "doc:"
        );
        
        std::cout << "Connected to RedisSearch\n";
        
        // Index some sample documents
        std::vector<SearchDocument> docs = {
            {
                .url = "https://example.com/ml-basics",
                .title = "Machine Learning Basics",
                .content = "An introduction to machine learning concepts, algorithms, and applications. Perfect for beginners.",
                .domain = "example.com",
                .keywords = {"machine learning", "basics", "tutorial"},
                .score = 1.0
            },
            {
                .url = "https://github.com/ml-project",
                .title = "Advanced Machine Learning Project",
                .content = "A comprehensive machine learning project with implementations of various algorithms in Python.",
                .domain = "github.com",
                .keywords = {"machine learning", "python", "advanced"},
                .score = 1.2
            }
        };
        
        // Index documents
        for (const auto& doc : docs) {
            storage->indexDocument(doc);
        }
        
        // Search and score
        auto searchResult = storage->searchSimple("machine learning", 10);
        if (searchResult.isSuccess()) {
            auto response = searchResult.getValue();
            std::cout << "\nFound " << response.results.size() << " results\n";
            
            // Convert to the format expected by scorer
            std::vector<std::unordered_map<std::string, std::string>> redisResults;
            for (const auto& result : response.results) {
                std::unordered_map<std::string, std::string> doc;
                doc["url"] = result.url;
                doc["title"] = result.title;
                doc["content"] = result.snippet;
                doc["domain"] = result.domain;
                doc["score"] = std::to_string(result.score);
                redisResults.push_back(doc);
            }
            
            // Score with our custom scorer
            auto scorer = SearchScorer::createRedisSearchScorer();
            auto scores = scorer->rankResults(redisResults, "machine learning");
            
            std::cout << "\nCustom Scoring Results:\n";
            for (const auto& score : scores) {
                std::cout << "- " << score.documentId << ": " << score.totalScore << "\n";
            }
        }
        
        // Clean up
        storage->dropIndex();
        
    } catch (const std::exception& e) {
        std::cout << "Redis integration failed: " << e.what() << "\n";
    }
}
#endif

int main() {
    LOG_INFO("Starting scoring demonstration");
    
    // Run basic scoring demo
    demonstrateScoring();
    
    // Run Redis integration if available
    #ifdef REDIS_AVAILABLE
    demonstrateRedisIntegration();
    #else
    std::cout << "\n\nRedis not available - skipping integration demo\n";
    #endif
    
    std::cout << "\n=== Demo Complete ===\n";
    
    return 0;
} 