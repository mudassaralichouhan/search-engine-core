# Search Engine Core - C++20 Implementation

This document describes the `search_core` module implementation for the Search
Engine API project.

## Overview

The `search_core` module provides a high-level, type-safe interface to
RedisSearch with the following components:

- **SearchClient**: RAII-safe, connection-pooled Redis wrapper
- **QueryParser**: DSL for converting user queries to RedisSearch syntax with
  AST
- **Scorer**: Pluggable ranking policy system with JSON configuration

## Architecture

```
search_core/
├── include/search_core/
│   ├── SearchClient.hpp    # Redis connection management
│   ├── QueryParser.hpp     # Query parsing and AST
│   └── Scorer.hpp          # Ranking configuration
└── src/
    ├── SearchClient.cpp    # Implementation
    ├── QueryParser.cpp     # Query processing
    └── Scorer.cpp          # Scoring logic
```

## Features

### SearchClient

- **Connection Pooling**: Round-robin pool of Redis connections for thread
  safety
- **Exception Safety**: Translates Redis errors to typed exceptions
- **PIMPL Pattern**: Hides redis-plus-plus implementation details
- **Thread Safe**: Multiple threads can search concurrently

```cpp
hatef::search::SearchClient client({
    .uri = "tcp://127.0.0.1:6379",
    .pool_size = 4
});

auto result = client.search("my_index", "hello world");
```

### QueryParser

- **Text Normalization**: Lowercase, Unicode NFKC, punctuation handling
- **Query Features**:
  - Exact phrases: `"quick brown fox"`
  - Boolean operators: `hello AND world`, `cat OR dog`
  - Field filters: `site:example.com` → `@domain:{example.com}`
- **AST Generation**: Builds typed Abstract Syntax Tree
- **Redis Translation**: Converts AST to RedisSearch query syntax

```cpp
hatef::search::QueryParser parser;
auto ast = parser.parse("\"quick brown\" AND site:example.com");
auto redis_query = parser.toRedisSyntax(*ast);
// Result: ("quick brown" @domain:{example.com})
```

### Scorer

- **JSON Configuration**: Hot-reloadable scoring parameters
- **Field Weights**: Customizable per-field boost values
- **Parameter Support**: Configurable ranking algorithms

```json
{
  "field_weights": {
    "title": 2.0,
    "body": 1.0
  },
  "offset_boost": 0.1
}
```

## Building

### Prerequisites

- CMake 3.24+
- C++20 compatible compiler (GCC 10+, Clang 12+)
- Redis server with RedisSearch module
- Dependencies via vcpkg:
  - `redis-plus-plus`
  - `hiredis`
  - `nlohmann-json`
  - `catch2` (for tests)

### Build Commands

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON

# Build library
cmake --build build --target search_core

# Build and run tests
cmake --build build --target test_search_client test_query_parser test_scorer
ctest --test-dir build --output-on-failure -L search_core
```

### Quick Build Script

```bash
./build_search_core.sh
```

## Testing

### Test Categories

1. **Unit Tests**: Fast, isolated component testing
   - `TestSearchClient.cpp`: Connection pooling, error handling
   - `TestQueryParser.cpp`: 10 test cases covering DSL features
   - `TestScorer.cpp`: Configuration loading and argument building

2. **Integration Tests**: Requires running Redis server
   - `TestExactSearchE2E.cpp`: End-to-end search scenarios
   - Performance testing with p95 latency measurement
   - Thread safety validation

### Running Tests

```bash
# Unit tests only
ctest -L "search_core" -E "integration"

# Integration tests (requires Redis)
redis-server &
ctest -L "integration"

# Performance tests
ctest -L "performance"
```

## Configuration

### Redis Configuration (`config/redis.json`)

```json
{
  "uri": "tcp://127.0.0.1:6379",
  "pool_size": 4,
  "timeout_ms": 5000,
  "retry_attempts": 3
}
```

### Scoring Configuration (`config/scoring.json`)

```json
{
  "field_weights": {
    "title": 2.0,
    "body": 1.0
  },
  "offset_boost": 0.1
}
```

## Usage Examples

### Basic Search

```cpp
#include "search_core/SearchClient.hpp"
#include "search_core/QueryParser.hpp"
#include "search_core/Scorer.hpp"

using namespace hatef::search;

// Initialize components
SearchClient client;
QueryParser parser;
Scorer scorer("config/scoring.json");

// Parse user query
auto ast = parser.parse("\"machine learning\" OR AI site:arxiv.org");
auto redis_query = parser.toRedisSyntax(*ast);

// Execute search with scoring
auto scoring_args = scorer.buildArgs();
auto result = client.search("papers_index", redis_query, scoring_args);
```

### Advanced Usage

```cpp
// Custom Redis configuration
RedisConfig config {
    .uri = "tcp://redis-cluster:6379",
    .pool_size = 8
};
SearchClient client(config);

// Runtime scoring updates
scorer.reload("config/custom_scoring.json");
auto new_args = scorer.buildArgs();
```

## Performance Characteristics

- **Connection Pool**: Eliminates connection overhead
- **Thread Safety**: Lock-free round-robin connection selection
- **Memory Efficiency**: PIMPL pattern reduces compile-time dependencies
- **Query Parsing**: Single-pass tokenization and AST building
- **Target Latency**: <5ms p95 for local Redis operations

## Error Handling

All components use typed exceptions derived from `std::runtime_error`:

```cpp
try {
    auto result = client.search("index", "query");
} catch (const SearchError& e) {
    // Handle Redis/search specific errors
    std::cerr << "Search failed: " << e.what() << std::endl;
}
```

## Thread Safety

- **SearchClient**: Thread-safe connection pooling
- **QueryParser**: Stateless, thread-safe
- **Scorer**: Read-only after construction, thread-safe

## Integration with Main Project

The `search_core` library integrates with the existing search engine:

```cmake
target_link_libraries(server PRIVATE search_core)
```

```cpp
#include "search_core/SearchClient.hpp"
// Use in HTTP handlers, background indexing, etc.
```

## Next Steps

The next implementation slice will wrap this `search_core` in a uWebSockets HTTP
endpoint:

```
GET /search?q="machine learning"&limit=10
```

This will provide a REST API interface for the search functionality while
keeping the core logic decoupled and testable.
