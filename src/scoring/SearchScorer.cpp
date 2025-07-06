#include "../../include/search_engine/scoring/SearchScorer.h"
#include "../../include/Logger.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <numeric>
#include <regex>
#include <chrono>
#include <unordered_set>

namespace search_engine {
namespace scoring {

// ===== ScoringConfig Implementation =====

ScoringConfig ScoringConfig::createDefault() {
    return ScoringConfig();  // Use default values
}

ScoringConfig ScoringConfig::createTitleHeavy() {
    ScoringConfig config;
    config.fieldWeights.title = 10.0;      // Double the title weight
    config.fieldWeights.description = 2.0;  // Reduce other weights
    config.fieldWeights.content = 0.5;
    config.boostFactors.titleMatchBoost = 2.0;
    return config;
}

ScoringConfig ScoringConfig::createContentHeavy() {
    ScoringConfig config;
    config.fieldWeights.title = 3.0;
    config.fieldWeights.content = 2.0;      // Increase content weight
    config.fieldWeights.description = 1.5;
    config.tfParams.maxTermFrequency = 20.0; // Allow higher term frequency
    return config;
}

ScoringConfig ScoringConfig::createBalanced() {
    ScoringConfig config;
    config.fieldWeights.title = 3.0;
    config.fieldWeights.description = 2.0;
    config.fieldWeights.content = 1.5;
    config.fieldWeights.keywords = 2.5;
    config.boostFactors.exactMatchBoost = 1.5;
    config.boostFactors.titleMatchBoost = 1.2;
    return config;
}

// ===== Base ScoringAlgorithm Implementation =====

std::vector<DocumentScore> ScoringAlgorithm::scoreDocuments(
    const std::vector<DocumentInfo>& docs,
    const QueryInfo& query,
    const ScoringConfig& config
) const {
    std::vector<DocumentScore> scores;
    scores.reserve(docs.size());
    
    for (const auto& doc : docs) {
        scores.push_back(scoreDocument(doc, query, config));
    }
    
    return scores;
}

double ScoringAlgorithm::calculateTermFrequency(int tf, const ScoringConfig::TFParams& params) const {
    if (tf == 0) return 0.0;
    
    double score = static_cast<double>(tf);
    
    // Apply maximum term frequency cap
    if (score > params.maxTermFrequency) {
        score = params.maxTermFrequency;
    }
    
    // Apply log normalization if enabled
    if (params.useLogNormalization) {
        score = std::log(1.0 + score);
    }
    
    return score;
}

double ScoringAlgorithm::calculateFieldWeight(const std::string& field, const ScoringConfig::FieldWeights& weights) const {
    if (field == "title") return weights.title;
    if (field == "description") return weights.description;
    if (field == "content") return weights.content;
    if (field == "keywords") return weights.keywords;
    if (field == "url") return weights.url;
    if (field == "domain") return weights.domain;
    return 1.0;  // Default weight
}

int ScoringAlgorithm::countTermOccurrences(const std::string& text, const std::string& term) const {
    if (text.empty() || term.empty()) return 0;
    
    // Convert both to lowercase for case-insensitive matching
    std::string lowerText = text;
    std::string lowerTerm = term;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    std::transform(lowerTerm.begin(), lowerTerm.end(), lowerTerm.begin(), ::tolower);
    
    int count = 0;
    size_t pos = 0;
    
    // Count word boundaries to avoid partial matches
    std::regex termRegex("\\b" + lowerTerm + "\\b");
    auto words_begin = std::sregex_iterator(lowerText.begin(), lowerText.end(), termRegex);
    auto words_end = std::sregex_iterator();
    
    count = std::distance(words_begin, words_end);
    
    return count;
}

bool ScoringAlgorithm::containsExactPhrase(const std::string& text, const std::string& phrase) const {
    if (text.empty() || phrase.empty()) return false;
    
    std::string lowerText = text;
    std::string lowerPhrase = phrase;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    std::transform(lowerPhrase.begin(), lowerPhrase.end(), lowerPhrase.begin(), ::tolower);
    
    return lowerText.find(lowerPhrase) != std::string::npos;
}

// ===== BM25Algorithm Implementation =====

DocumentScore BM25Algorithm::scoreDocument(
    const DocumentInfo& doc,
    const QueryInfo& query,
    const ScoringConfig& config
) const {
    DocumentScore score;
    score.documentId = doc.id;
    
    // Calculate scores for each field
    for (const auto& term : query.terms) {
        // Title scoring
        int titleTF = doc.titleTermFreq.count(term) ? doc.titleTermFreq.at(term) : 
                      countTermOccurrences(doc.title, term);
        if (titleTF > 0) {
            double fieldScore = calculateBM25Score(
                titleTF, 
                doc.title.length(), 
                documentFrequencies_[term],
                config.bm25Params
            );
            score.titleScore += fieldScore * calculateFieldWeight("title", config.fieldWeights);
            score.matchedTerms++;
        }
        
        // Content scoring
        int contentTF = doc.contentTermFreq.count(term) ? doc.contentTermFreq.at(term) : 
                        countTermOccurrences(doc.content, term);
        if (contentTF > 0) {
            double fieldScore = calculateBM25Score(
                contentTF, 
                doc.content.length(), 
                documentFrequencies_[term],
                config.bm25Params
            );
            score.contentScore += fieldScore * calculateFieldWeight("content", config.fieldWeights);
        }
        
        // Description scoring
        if (!doc.description.empty()) {
            int descTF = doc.descriptionTermFreq.count(term) ? doc.descriptionTermFreq.at(term) : 
                         countTermOccurrences(doc.description, term);
            if (descTF > 0) {
                double fieldScore = calculateBM25Score(
                    descTF, 
                    doc.description.length(), 
                    documentFrequencies_[term],
                    config.bm25Params
                );
                score.descriptionScore += fieldScore * calculateFieldWeight("description", config.fieldWeights);
            }
        }
        
        // Keyword scoring
        for (const auto& keyword : doc.keywords) {
            if (countTermOccurrences(keyword, term) > 0) {
                score.keywordScore += 1.0 * calculateFieldWeight("keywords", config.fieldWeights);
                break;
            }
        }
    }
    
    // Calculate total terms and coverage
    score.totalTerms = query.terms.size();
    score.coverage = score.matchedTerms / static_cast<double>(score.totalTerms);
    
    // Check for exact phrase matches
    for (const auto& phrase : query.exactPhrases) {
        if (containsExactPhrase(doc.title, phrase)) {
            score.exactMatchScore += config.boostFactors.exactMatchBoost * calculateFieldWeight("title", config.fieldWeights);
        }
        if (containsExactPhrase(doc.content, phrase)) {
            score.exactMatchScore += config.boostFactors.exactMatchBoost * calculateFieldWeight("content", config.fieldWeights);
        }
    }
    
    // Combine all scores
    score.termFrequencyScore = score.titleScore + score.contentScore + score.descriptionScore + score.keywordScore;
    score.totalScore = score.termFrequencyScore + score.exactMatchScore;
    
    // Apply base score from RedisSearch if available
    if (doc.baseScore > 0) {
        score.totalScore *= doc.baseScore;
    }
    
    // Generate explanation
    std::ostringstream oss;
    oss << "BM25 Score: " << score.totalScore 
        << " (Title: " << score.titleScore 
        << ", Content: " << score.contentScore
        << ", Exact: " << score.exactMatchScore
        << ", Coverage: " << (score.coverage * 100) << "%)";
    score.explanation = oss.str();
    
    return score;
}

void BM25Algorithm::updateCorpusStatistics(const std::vector<DocumentInfo>& corpus) {
    if (corpus.empty()) return;
    
    totalDocuments_ = corpus.size();
    documentFrequencies_.clear();
    
    // Calculate average document length
    double totalLength = 0.0;
    for (const auto& doc : corpus) {
        totalLength += doc.content.length() + doc.title.length() + doc.description.length();
        
        // Update document frequencies
        std::unordered_set<std::string> docTerms;
        auto titleTerms = scoring_utils::extractTerms(doc.title);
        auto contentTerms = scoring_utils::extractTerms(doc.content);
        auto descTerms = scoring_utils::extractTerms(doc.description);
        
        docTerms.insert(titleTerms.begin(), titleTerms.end());
        docTerms.insert(contentTerms.begin(), contentTerms.end());
        docTerms.insert(descTerms.begin(), descTerms.end());
        
        for (const auto& term : docTerms) {
            documentFrequencies_[term]++;
        }
    }
    
    avgDocumentLength_ = totalLength / totalDocuments_;
    
    LOG_INFO("Updated BM25 corpus statistics: " + std::to_string(totalDocuments_) + 
             " documents, avg length: " + std::to_string(avgDocumentLength_));
}

double BM25Algorithm::calculateBM25Score(
    int termFreq,
    int docLength,
    size_t docFreq,
    const ScoringConfig::BM25Params& params
) const {
    if (termFreq == 0 || totalDocuments_ == 0) return 0.0;
    
    // IDF component: log((N - df + 0.5) / (df + 0.5))
    double idf = std::log((totalDocuments_ - docFreq + 0.5) / (docFreq + 0.5));
    
    // TF component: (tf * (k1 + 1)) / (tf + k1 * (1 - b + b * (docLen / avgDocLen)))
    double normalizedLength = docLength / avgDocumentLength_;
    double tfComponent = (termFreq * (params.k1 + 1)) / 
                        (termFreq + params.k1 * (1 - params.b + params.b * normalizedLength));
    
    return idf * tfComponent;
}

// ===== TFIDFAlgorithm Implementation =====

DocumentScore TFIDFAlgorithm::scoreDocument(
    const DocumentInfo& doc,
    const QueryInfo& query,
    const ScoringConfig& config
) const {
    DocumentScore score;
    score.documentId = doc.id;
    
    // Calculate TF-IDF for each query term in each field
    for (const auto& term : query.terms) {
        double termIDF = calculateIDF(term);
        
        // Title scoring
        int titleTF = countTermOccurrences(doc.title, term);
        if (titleTF > 0) {
            double tf = calculateTermFrequency(titleTF, config.tfParams);
            if (config.tfParams.normalizeByLength && doc.title.length() > 0) {
                tf /= doc.title.length();
            }
            score.titleScore += tf * termIDF * calculateFieldWeight("title", config.fieldWeights);
            score.matchedTerms++;
        }
        
        // Content scoring
        int contentTF = countTermOccurrences(doc.content, term);
        if (contentTF > 0) {
            double tf = calculateTermFrequency(contentTF, config.tfParams);
            if (config.tfParams.normalizeByLength && doc.content.length() > 0) {
                tf /= doc.content.length();
            }
            score.contentScore += tf * termIDF * calculateFieldWeight("content", config.fieldWeights);
        }
        
        // Description scoring
        if (!doc.description.empty()) {
            int descTF = countTermOccurrences(doc.description, term);
            if (descTF > 0) {
                double tf = calculateTermFrequency(descTF, config.tfParams);
                if (config.tfParams.normalizeByLength && doc.description.length() > 0) {
                    tf /= doc.description.length();
                }
                score.descriptionScore += tf * termIDF * calculateFieldWeight("description", config.fieldWeights);
            }
        }
    }
    
    // Calculate coverage and exact matches
    score.totalTerms = query.terms.size();
    score.coverage = score.matchedTerms / static_cast<double>(score.totalTerms);
    
    // Check for exact phrase matches
    for (const auto& phrase : query.exactPhrases) {
        if (containsExactPhrase(doc.title, phrase)) {
            score.exactMatchScore += config.boostFactors.exactMatchBoost;
        }
        if (containsExactPhrase(doc.content, phrase)) {
            score.exactMatchScore += config.boostFactors.exactMatchBoost * 0.5;
        }
    }
    
    // Combine scores
    score.termFrequencyScore = score.titleScore + score.contentScore + score.descriptionScore;
    score.totalScore = score.termFrequencyScore + score.exactMatchScore;
    
    // Apply base score from RedisSearch
    if (doc.baseScore > 0) {
        score.totalScore *= doc.baseScore;
    }
    
    // Generate explanation
    std::ostringstream oss;
    oss << "TF-IDF Score: " << score.totalScore 
        << " (Title: " << score.titleScore 
        << ", Content: " << score.contentScore
        << ", Exact: " << score.exactMatchScore << ")";
    score.explanation = oss.str();
    
    return score;
}

void TFIDFAlgorithm::updateDocumentFrequencies(const std::vector<DocumentInfo>& corpus) {
    totalDocuments_ = corpus.size();
    documentFrequencies_.clear();
    
    for (const auto& doc : corpus) {
        std::unordered_set<std::string> docTerms;
        
        auto titleTerms = scoring_utils::extractTerms(doc.title);
        auto contentTerms = scoring_utils::extractTerms(doc.content);
        
        docTerms.insert(titleTerms.begin(), titleTerms.end());
        docTerms.insert(contentTerms.begin(), contentTerms.end());
        
        for (const auto& term : docTerms) {
            documentFrequencies_[term]++;
        }
    }
}

double TFIDFAlgorithm::calculateIDF(const std::string& term) const {
    if (totalDocuments_ == 0) return 0.0;
    
    size_t docFreq = documentFrequencies_.count(term) ? documentFrequencies_.at(term) : 1;
    return std::log(static_cast<double>(totalDocuments_) / docFreq);
}

// ===== RedisSearchCombinedAlgorithm Implementation =====

RedisSearchCombinedAlgorithm::RedisSearchCombinedAlgorithm(std::unique_ptr<ScoringAlgorithm> baseAlgo)
    : baseAlgorithm_(std::move(baseAlgo)) {
    if (!baseAlgorithm_) {
        baseAlgorithm_ = std::make_unique<BM25Algorithm>();
    }
}

DocumentScore RedisSearchCombinedAlgorithm::scoreDocument(
    const DocumentInfo& doc,
    const QueryInfo& query,
    const ScoringConfig& config
) const {
    // Get base algorithm score
    DocumentScore score = baseAlgorithm_->scoreDocument(doc, query, config);
    
    // Combine with RedisSearch score using weighted average
    double redisWeight = 0.3;  // 30% Redis score, 70% custom algorithm
    double customWeight = 0.7;
    
    if (doc.baseScore > 0) {
        score.totalScore = (doc.baseScore * redisWeight) + (score.totalScore * customWeight);
        
        // Update explanation
        score.explanation = "Combined(" + std::to_string(redisWeight * 100) + "% Redis + " +
                           std::to_string(customWeight * 100) + "% " + baseAlgorithm_->getName() + 
                           "): " + std::to_string(score.totalScore);
    }
    
    return score;
}

// ===== SearchScorer Implementation =====

SearchScorer::SearchScorer(const ScoringConfig& config) 
    : config_(config) {
    // Default to BM25 algorithm
    algorithm_ = std::make_unique<BM25Algorithm>();
}

void SearchScorer::setAlgorithm(std::unique_ptr<ScoringAlgorithm> algorithm) {
    if (algorithm) {
        algorithm_ = std::move(algorithm);
    }
}

DocumentInfo SearchScorer::extractDocumentInfo(const std::unordered_map<std::string, std::string>& redisDoc) const {
    DocumentInfo info;
    
    // Extract fields from Redis document
    if (redisDoc.count("url")) info.url = redisDoc.at("url");
    if (redisDoc.count("title")) info.title = redisDoc.at("title");
    if (redisDoc.count("content")) info.content = redisDoc.at("content");
    if (redisDoc.count("description")) info.description = redisDoc.at("description");
    if (redisDoc.count("domain")) info.domain = redisDoc.at("domain");
    if (redisDoc.count("score")) {
        try {
            info.baseScore = std::stod(redisDoc.at("score"));
        } catch (...) {
            info.baseScore = 1.0;
        }
    }
    
    // Extract keywords
    if (redisDoc.count("keywords")) {
        std::istringstream iss(redisDoc.at("keywords"));
        std::string keyword;
        while (std::getline(iss, keyword, '|')) {
            if (!keyword.empty()) {
                info.keywords.push_back(keyword);
            }
        }
    }
    
    // Calculate content length
    info.contentLength = info.content.length();
    
    // Pre-calculate term frequencies for efficiency
    info.titleTermFreq = scoring_utils::calculateTermFrequencies(info.title);
    info.contentTermFreq = scoring_utils::calculateTermFrequencies(info.content);
    info.descriptionTermFreq = scoring_utils::calculateTermFrequencies(info.description);
    
    // Generate document ID if not present
    if (info.id.empty()) {
        info.id = info.url;
    }
    
    return info;
}

QueryInfo SearchScorer::extractQueryInfo(const std::string& query) const {
    QueryInfo info;
    
    // Extract terms (basic tokenization)
    info.terms = scoring_utils::extractTerms(query);
    
    // Extract exact phrases (text within quotes)
    std::regex phraseRegex("\"([^\"]+)\"");
    std::smatch match;
    std::string searchText = query;
    
    while (std::regex_search(searchText, match, phraseRegex)) {
        info.exactPhrases.push_back(match[1].str());
        searchText = match.suffix();
    }
    
    // Set default term weights
    for (const auto& term : info.terms) {
        info.termWeights[term] = 1.0;
    }
    
    // Check for AND query (all terms required)
    info.requireAllTerms = (query.find(" AND ") != std::string::npos);
    
    return info;
}

void SearchScorer::applyBoosts(DocumentScore& score, const DocumentInfo& doc, const QueryInfo& query) const {
    // Apply domain authority boost
    if (doc.domain == "github.com" || doc.domain == "stackoverflow.com") {
        score.boostScore += config_.boostFactors.domainAuthorityBoost;
    }
    
    // Apply title match boost
    bool hasTitleMatch = false;
    for (const auto& term : query.terms) {
        if (doc.title.find(term) != std::string::npos) {
            hasTitleMatch = true;
            break;
        }
    }
    if (hasTitleMatch) {
        score.boostScore += config_.boostFactors.titleMatchBoost;
    }
    
    // Apply freshness boost (if indexedAt is available)
    // This would need to be implemented based on doc.indexedAt
    
    // Apply boost to total score
    score.totalScore *= (1.0 + score.boostScore);
}

void SearchScorer::normalizeScores(std::vector<DocumentScore>& scores) const {
    if (scores.empty() || !config_.normalizeScores) return;
    
    // Find min and max scores
    double minScore = scores[0].totalScore;
    double maxScore = scores[0].totalScore;
    
    for (const auto& score : scores) {
        minScore = std::min(minScore, score.totalScore);
        maxScore = std::max(maxScore, score.totalScore);
    }
    
    // Normalize to [0, 1] range
    double range = maxScore - minScore;
    if (range > 0) {
        for (auto& score : scores) {
            score.totalScore = (score.totalScore - minScore) / range;
        }
    }
}

std::string SearchScorer::generateExplanation(const DocumentScore& score, const DocumentInfo& doc, const QueryInfo& query) const {
    std::ostringstream oss;
    oss << "Document: " << doc.url << "\n";
    oss << "Algorithm: " << algorithm_->getName() << "\n";
    oss << "Total Score: " << score.totalScore << "\n";
    oss << "Field Scores: Title=" << score.titleScore 
        << ", Content=" << score.contentScore 
        << ", Description=" << score.descriptionScore << "\n";
    oss << "Matched Terms: " << score.matchedTerms << "/" << score.totalTerms 
        << " (Coverage: " << (score.coverage * 100) << "%)\n";
    oss << "Boosts Applied: " << score.boostScore << "\n";
    return oss.str();
}

std::vector<DocumentScore> SearchScorer::scoreResults(
    const std::vector<std::unordered_map<std::string, std::string>>& redisResults,
    const std::string& query
) const {
    // Extract query information
    QueryInfo queryInfo = extractQueryInfo(query);
    
    // Convert Redis results to DocumentInfo
    std::vector<DocumentInfo> documents;
    documents.reserve(redisResults.size());
    
    for (const auto& redisDoc : redisResults) {
        documents.push_back(extractDocumentInfo(redisDoc));
    }
    
    // Score all documents
    std::vector<DocumentScore> scores = algorithm_->scoreDocuments(documents, queryInfo, config_);
    
    // Apply boosts
    for (size_t i = 0; i < scores.size(); ++i) {
        applyBoosts(scores[i], documents[i], queryInfo);
    }
    
    // Filter out low scores
    scores.erase(
        std::remove_if(scores.begin(), scores.end(),
                      [this](const DocumentScore& s) { return s.totalScore < config_.minScore; }),
        scores.end()
    );
    
    // Normalize scores if configured
    normalizeScores(scores);
    
    // Add detailed explanations
    for (size_t i = 0; i < scores.size(); ++i) {
        scores[i].explanation = generateExplanation(scores[i], documents[i], queryInfo);
    }
    
    return scores;
}

std::vector<DocumentScore> SearchScorer::rankResults(
    const std::vector<std::unordered_map<std::string, std::string>>& redisResults,
    const std::string& query,
    size_t topK
) const {
    // Score all results
    std::vector<DocumentScore> scores = scoreResults(redisResults, query);
    
    // Sort by score (descending)
    std::sort(scores.begin(), scores.end(), std::greater<DocumentScore>());
    
    // Return top K results
    if (topK > 0 && scores.size() > topK) {
        scores.resize(topK);
    }
    
    LOG_DEBUG("Ranked " + std::to_string(scores.size()) + " results for query: " + query);
    
    return scores;
}

// Factory methods
std::unique_ptr<SearchScorer> SearchScorer::createBM25Scorer(const ScoringConfig& config) {
    auto scorer = std::make_unique<SearchScorer>(config);
    scorer->setAlgorithm(std::make_unique<BM25Algorithm>());
    return scorer;
}

std::unique_ptr<SearchScorer> SearchScorer::createTFIDFScorer(const ScoringConfig& config) {
    auto scorer = std::make_unique<SearchScorer>(config);
    scorer->setAlgorithm(std::make_unique<TFIDFAlgorithm>());
    return scorer;
}

std::unique_ptr<SearchScorer> SearchScorer::createRedisSearchScorer(const ScoringConfig& config) {
    auto scorer = std::make_unique<SearchScorer>(config);
    scorer->setAlgorithm(std::make_unique<RedisSearchCombinedAlgorithm>(
        std::make_unique<BM25Algorithm>()
    ));
    return scorer;
}

// ===== Utility Functions Implementation =====

namespace scoring_utils {

double calculateJaccardSimilarity(
    const std::vector<std::string>& terms1,
    const std::vector<std::string>& terms2
) {
    if (terms1.empty() || terms2.empty()) return 0.0;
    
    std::unordered_set<std::string> set1(terms1.begin(), terms1.end());
    std::unordered_set<std::string> set2(terms2.begin(), terms2.end());
    
    std::vector<std::string> intersection;
    std::set_intersection(set1.begin(), set1.end(), set2.begin(), set2.end(),
                         std::back_inserter(intersection));
    
    std::unordered_set<std::string> unionSet(set1);
    unionSet.insert(set2.begin(), set2.end());
    
    return static_cast<double>(intersection.size()) / unionSet.size();
}

double calculateCosineSimilarity(
    const std::unordered_map<std::string, double>& vec1,
    const std::unordered_map<std::string, double>& vec2
) {
    double dotProduct = 0.0;
    double norm1 = 0.0;
    double norm2 = 0.0;
    
    for (const auto& [term, weight] : vec1) {
        norm1 += weight * weight;
        if (vec2.count(term)) {
            dotProduct += weight * vec2.at(term);
        }
    }
    
    for (const auto& [term, weight] : vec2) {
        norm2 += weight * weight;
    }
    
    if (norm1 == 0.0 || norm2 == 0.0) return 0.0;
    
    return dotProduct / (std::sqrt(norm1) * std::sqrt(norm2));
}

std::vector<std::string> extractTerms(const std::string& text) {
    std::vector<std::string> terms;
    std::string current;
    
    for (char c : text) {
        if (std::isalnum(c)) {
            current += std::tolower(c);
        } else if (!current.empty()) {
            if (current.length() >= 2) {  // Skip single characters
                terms.push_back(current);
            }
            current.clear();
        }
    }
    
    if (!current.empty() && current.length() >= 2) {
        terms.push_back(current);
    }
    
    return terms;
}

std::unordered_map<std::string, int> calculateTermFrequencies(const std::string& text) {
    std::unordered_map<std::string, int> frequencies;
    auto terms = extractTerms(text);
    
    for (const auto& term : terms) {
        frequencies[term]++;
    }
    
    return frequencies;
}

void normalizeScores(std::vector<double>& scores) {
    if (scores.empty()) return;
    
    double minScore = *std::min_element(scores.begin(), scores.end());
    double maxScore = *std::max_element(scores.begin(), scores.end());
    
    double range = maxScore - minScore;
    if (range > 0) {
        for (auto& score : scores) {
            score = (score - minScore) / range;
        }
    }
}

} // namespace scoring_utils
} // namespace scoring
} // namespace search_engine 