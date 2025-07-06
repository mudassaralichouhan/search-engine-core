#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <cmath>
#include <chrono>

namespace search_engine {
namespace scoring {

// Forward declarations
struct ScoringConfig;
struct DocumentScore;
class ScoringAlgorithm;

// Configuration for scoring and ranking
struct ScoringConfig {
    // Field weights - how important each field is
    struct FieldWeights {
        double title = 5.0;
        double description = 3.0;
        double content = 1.0;
        double keywords = 4.0;
        double url = 0.5;
        double domain = 0.8;
    } fieldWeights;
    
    // Term frequency parameters
    struct TFParams {
        bool useLogNormalization = true;  // Use log(1 + tf) instead of raw tf
        double maxTermFrequency = 10.0;   // Cap term frequency to avoid spam
        bool normalizeByLength = true;    // Normalize by document length
    } tfParams;
    
    // BM25 specific parameters
    struct BM25Params {
        double k1 = 1.2;  // Term frequency saturation parameter
        double b = 0.75;  // Length normalization parameter
    } bm25Params;
    
    // Boost factors
    struct BoostFactors {
        double exactMatchBoost = 2.0;      // Boost for exact phrase matches
        double titleMatchBoost = 1.5;      // Additional boost for title matches
        double domainAuthorityBoost = 1.2; // Boost for authoritative domains
        double freshnessBoost = 1.1;       // Boost for recent documents
    } boostFactors;
    
    // Scoring thresholds
    double minScore = 0.01;              // Minimum score to include in results
    bool normalizeScores = true;         // Normalize final scores to [0, 1]
    
    // Create default configuration
    static ScoringConfig createDefault();
    static ScoringConfig createTitleHeavy();    // Emphasize title matches
    static ScoringConfig createContentHeavy();  // Emphasize content matches
    static ScoringConfig createBalanced();      // Balanced scoring
};

// Document information for scoring
struct DocumentInfo {
    std::string id;
    std::string url;
    std::string title;
    std::string content;
    std::string description;
    std::vector<std::string> keywords;
    std::string domain;
    
    // Metadata
    size_t contentLength = 0;
    double baseScore = 1.0;  // RedisSearch base score
    std::chrono::system_clock::time_point indexedAt;
    
    // Term frequency maps for each field
    std::unordered_map<std::string, int> titleTermFreq;
    std::unordered_map<std::string, int> contentTermFreq;
    std::unordered_map<std::string, int> descriptionTermFreq;
};

// Query information for scoring
struct QueryInfo {
    std::vector<std::string> terms;           // Query terms
    std::vector<std::string> exactPhrases;    // Exact phrases to match
    std::unordered_map<std::string, double> termWeights; // Per-term weights
    bool requireAllTerms = false;             // AND vs OR query
};

// Detailed scoring breakdown
struct DocumentScore {
    std::string documentId;
    double totalScore = 0.0;
    
    // Field-specific scores
    double titleScore = 0.0;
    double contentScore = 0.0;
    double descriptionScore = 0.0;
    double keywordScore = 0.0;
    double urlScore = 0.0;
    
    // Component scores
    double termFrequencyScore = 0.0;
    double fieldWeightScore = 0.0;
    double exactMatchScore = 0.0;
    double boostScore = 0.0;
    
    // Metadata
    int matchedTerms = 0;
    int totalTerms = 0;
    double coverage = 0.0;  // Percentage of query terms matched
    
    // Debug information
    std::string explanation;
    
    // Comparison operator for sorting
    bool operator>(const DocumentScore& other) const {
        return totalScore > other.totalScore;
    }
};

// Base class for scoring algorithms
class ScoringAlgorithm {
public:
    virtual ~ScoringAlgorithm() = default;
    
    // Score a single document against a query
    virtual DocumentScore scoreDocument(
        const DocumentInfo& doc,
        const QueryInfo& query,
        const ScoringConfig& config
    ) const = 0;
    
    // Score multiple documents (can be optimized in bulk)
    virtual std::vector<DocumentScore> scoreDocuments(
        const std::vector<DocumentInfo>& docs,
        const QueryInfo& query,
        const ScoringConfig& config
    ) const;
    
    // Get algorithm name
    virtual std::string getName() const = 0;
    
protected:
    // Helper methods
    double calculateTermFrequency(int tf, const ScoringConfig::TFParams& params) const;
    double calculateFieldWeight(const std::string& field, const ScoringConfig::FieldWeights& weights) const;
    int countTermOccurrences(const std::string& text, const std::string& term) const;
    bool containsExactPhrase(const std::string& text, const std::string& phrase) const;
};

// BM25 scoring algorithm
class BM25Algorithm : public ScoringAlgorithm {
private:
    // Corpus statistics (should be updated periodically)
    mutable double avgDocumentLength_ = 100.0;
    mutable size_t totalDocuments_ = 1000;
    mutable std::unordered_map<std::string, size_t> documentFrequencies_;
    
public:
    DocumentScore scoreDocument(
        const DocumentInfo& doc,
        const QueryInfo& query,
        const ScoringConfig& config
    ) const override;
    
    std::string getName() const override { return "BM25"; }
    
    // Update corpus statistics
    void updateCorpusStatistics(const std::vector<DocumentInfo>& corpus);
    
private:
    double calculateBM25Score(
        int termFreq,
        int docLength,
        size_t docFreq,
        const ScoringConfig::BM25Params& params
    ) const;
};

// TF-IDF scoring algorithm
class TFIDFAlgorithm : public ScoringAlgorithm {
private:
    mutable size_t totalDocuments_ = 1000;
    mutable std::unordered_map<std::string, size_t> documentFrequencies_;
    
public:
    DocumentScore scoreDocument(
        const DocumentInfo& doc,
        const QueryInfo& query,
        const ScoringConfig& config
    ) const override;
    
    std::string getName() const override { return "TF-IDF"; }
    
    void updateDocumentFrequencies(const std::vector<DocumentInfo>& corpus);
    
private:
    double calculateIDF(const std::string& term) const;
};

// Combined scoring using RedisSearch base score
class RedisSearchCombinedAlgorithm : public ScoringAlgorithm {
private:
    std::unique_ptr<ScoringAlgorithm> baseAlgorithm_;
    
public:
    explicit RedisSearchCombinedAlgorithm(std::unique_ptr<ScoringAlgorithm> baseAlgo);
    
    DocumentScore scoreDocument(
        const DocumentInfo& doc,
        const QueryInfo& query,
        const ScoringConfig& config
    ) const override;
    
    std::string getName() const override { 
        return "RedisSearchCombined+" + baseAlgorithm_->getName(); 
    }
};

// Main scorer class that orchestrates the scoring process
class SearchScorer {
private:
    std::unique_ptr<ScoringAlgorithm> algorithm_;
    ScoringConfig config_;
    
    // Helper methods
    DocumentInfo extractDocumentInfo(const std::unordered_map<std::string, std::string>& redisDoc) const;
    QueryInfo extractQueryInfo(const std::string& query) const;
    void applyBoosts(DocumentScore& score, const DocumentInfo& doc, const QueryInfo& query) const;
    void normalizeScores(std::vector<DocumentScore>& scores) const;
    std::string generateExplanation(const DocumentScore& score, const DocumentInfo& doc, const QueryInfo& query) const;
    
public:
    explicit SearchScorer(const ScoringConfig& config = ScoringConfig::createDefault());
    
    // Set the scoring algorithm
    void setAlgorithm(std::unique_ptr<ScoringAlgorithm> algorithm);
    void setConfig(const ScoringConfig& config) { config_ = config; }
    
    // Score documents from RedisSearch results
    std::vector<DocumentScore> scoreResults(
        const std::vector<std::unordered_map<std::string, std::string>>& redisResults,
        const std::string& query
    ) const;
    
    // Score and rank documents
    std::vector<DocumentScore> rankResults(
        const std::vector<std::unordered_map<std::string, std::string>>& redisResults,
        const std::string& query,
        size_t topK = 0  // 0 = return all
    ) const;
    
    // Get current configuration
    const ScoringConfig& getConfig() const { return config_; }
    std::string getAlgorithmName() const { return algorithm_->getName(); }
    
    // Factory methods
    static std::unique_ptr<SearchScorer> createBM25Scorer(const ScoringConfig& config = ScoringConfig::createDefault());
    static std::unique_ptr<SearchScorer> createTFIDFScorer(const ScoringConfig& config = ScoringConfig::createDefault());
    static std::unique_ptr<SearchScorer> createRedisSearchScorer(const ScoringConfig& config = ScoringConfig::createDefault());
};

// Utility functions for scoring
namespace scoring_utils {
    // Calculate Jaccard similarity between two sets of terms
    double calculateJaccardSimilarity(
        const std::vector<std::string>& terms1,
        const std::vector<std::string>& terms2
    );
    
    // Calculate cosine similarity between term vectors
    double calculateCosineSimilarity(
        const std::unordered_map<std::string, double>& vec1,
        const std::unordered_map<std::string, double>& vec2
    );
    
    // Extract terms from text with basic normalization
    std::vector<std::string> extractTerms(const std::string& text);
    
    // Calculate term frequencies for a text
    std::unordered_map<std::string, int> calculateTermFrequencies(const std::string& text);
    
    // Normalize scores to [0, 1] range
    void normalizeScores(std::vector<double>& scores);
}

} // namespace scoring
} // namespace search_engine 