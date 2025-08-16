# Search Result Scoring and Ranking

This document describes the scoring and ranking system that ranks search results
based on term frequency, field weights, and RedisSearch's built-in scoring.

## Overview

The scoring system provides multiple algorithms and customizable configurations
to rank search results effectively:

- **BM25 Algorithm** - State-of-the-art probabilistic ranking function
- **TF-IDF Algorithm** - Classic term frequency-inverse document frequency
  scoring
- **Combined Scoring** - Integrates custom algorithms with RedisSearch's
  built-in scores
- **Customizable Field Weights** - Title > Body > Other fields
- **Boost Factors** - Exact matches, domain authority, freshness

## Key Features

### 1. Field Weighting (Title > Body)

The system implements configurable field weights where title matches are valued
more than body content:

```cpp
struct FieldWeights {
    double title = 5.0;        // Highest weight
    double keywords = 4.0;
    double description = 3.0;
    double content = 1.0;      // Body content
    double url = 0.5;
    double domain = 0.8;
};
```

### 2. Term Frequency Scoring

Term frequency is calculated with normalization options:

```cpp
struct TFParams {
    bool useLogNormalization = true;   // log(1 + tf)
    double maxTermFrequency = 10.0;    // Cap to prevent spam
    bool normalizeByLength = true;     // Normalize by document length
};
```

### 3. BM25 Algorithm

The BM25 implementation uses:

- **k1 = 1.2** - Term frequency saturation
- **b = 0.75** - Length normalization
- IDF calculation: `log((N - df + 0.5) / (df + 0.5))`

### 4. RedisSearch Integration

The system can combine custom scoring with RedisSearch's built-in scores:

```cpp
// 30% Redis score + 70% custom algorithm
double redisWeight = 0.3;
double customWeight = 0.7;
finalScore = (redisScore * redisWeight) + (customScore * customWeight);
```

## Usage Examples

### Basic Usage

```cpp
#include "search_engine/scoring/SearchScorer.h"

// Create a scorer with BM25 algorithm
auto scorer = SearchScorer::createBM25Scorer();

// Score Redis results
std::vector<std::unordered_map<std::string, std::string>> redisResults = ...;
auto scores = scorer->rankResults(redisResults, "machine learning", 10);

// Results are sorted by score (highest first)
for (const auto& score : scores) {
    std::cout << "Document: " << score.documentId << "\n";
    std::cout << "Total Score: " << score.totalScore << "\n";
    std::cout << "Title Score: " << score.titleScore << "\n";
    std::cout << "Content Score: " << score.contentScore << "\n";
}
```

### Custom Configuration

```cpp
// Create custom scoring configuration
ScoringConfig config;
config.fieldWeights.title = 10.0;     // Heavily favor titles
config.fieldWeights.content = 0.5;    // Reduce content weight
config.boostFactors.exactMatchBoost = 3.0;  // Triple exact matches

// Create scorer with custom config
auto scorer = SearchScorer::createBM25Scorer(config);
```

### Using Different Algorithms

```cpp
// BM25 (recommended)
auto bm25Scorer = SearchScorer::createBM25Scorer();

// TF-IDF
auto tfidfScorer = SearchScorer::createTFIDFScorer();

// Combined with RedisSearch
auto combinedScorer = SearchScorer::createRedisSearchScorer();
```

## Configuration Presets

### Default Configuration

- Balanced weights across fields
- Standard BM25 parameters
- Moderate boost factors

### Title-Heavy Configuration

```cpp
auto config = ScoringConfig::createTitleHeavy();
// title weight = 10.0, content weight = 0.5
```

### Content-Heavy Configuration

```cpp
auto config = ScoringConfig::createContentHeavy();
// title weight = 3.0, content weight = 2.0
```

### Balanced Configuration

```cpp
auto config = ScoringConfig::createBalanced();
// Moderate weights across all fields
```

## Scoring Components

### 1. Term Frequency Score

- Counts occurrences of query terms
- Applies log normalization
- Caps maximum frequency

### 2. Field Weight Score

- Multiplies term scores by field importance
- Title matches get 5x boost by default

### 3. Exact Match Score

- Detects quoted phrases
- Applies exactMatchBoost (2x by default)

### 4. Boost Score

- Domain authority (github.com, stackoverflow.com)
- Title match boost
- Freshness boost (if timestamp available)

### 5. Coverage Score

- Percentage of query terms matched
- Used for relevance assessment

## Integration with RedisSearch

The scorer expects Redis results in this format:

```cpp
std::unordered_map<std::string, std::string> redisDoc = {
    {"url", "https://example.com/page"},
    {"title", "Page Title"},
    {"content", "Page content..."},
    {"description", "Page description"},
    {"keywords", "keyword1|keyword2|keyword3"},  // Pipe-separated
    {"domain", "example.com"},
    {"score", "0.85"}  // RedisSearch base score
};
```

## Performance Characteristics

- **BM25 Scoring**: ~10-20 microseconds per document
- **Memory Usage**: O(n) where n is number of unique terms
- **Scalability**: Linear with document count

## Advanced Features

### Corpus Statistics

For accurate BM25/TF-IDF scoring, update corpus statistics:

```cpp
BM25Algorithm* bm25 = dynamic_cast<BM25Algorithm*>(scorer->algorithm_.get());
if (bm25) {
    bm25->updateCorpusStatistics(allDocuments);
}
```

### Score Normalization

Enable score normalization to get scores in [0, 1] range:

```cpp
config.normalizeScores = true;
```

### Detailed Explanations

Get scoring breakdown for debugging:

```cpp
auto scores = scorer->scoreResults(results, query);
for (const auto& score : scores) {
    std::cout << score.explanation << "\n";
}
```

## Testing

Comprehensive unit tests are provided in `tests/scoring/test_search_scorer.cpp`:

- Configuration tests
- BM25 scoring tests
- TF-IDF scoring tests
- Combined scoring tests
- Field weighting tests
- Edge case handling

Run tests:

```bash
./build/tests/scoring/test_search_scorer
```

## Customization

To implement a custom scoring algorithm:

```cpp
class MyCustomAlgorithm : public ScoringAlgorithm {
public:
    DocumentScore scoreDocument(
        const DocumentInfo& doc,
        const QueryInfo& query,
        const ScoringConfig& config
    ) const override {
        // Implement custom scoring logic
    }

    std::string getName() const override { return "MyCustom"; }
};

// Use it
auto scorer = std::make_unique<SearchScorer>();
scorer->setAlgorithm(std::make_unique<MyCustomAlgorithm>());
```

## Future Enhancements

- **Machine Learning Scoring**: Neural ranking models
- **Personalized Scoring**: User-specific preferences
- **Semantic Scoring**: Vector similarity for semantic search
- **Click-through Rate Integration**: Learn from user behavior
- **A/B Testing Framework**: Compare scoring algorithms
