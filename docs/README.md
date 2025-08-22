# Search Engine Core Documentation

Welcome to the Search Engine Core documentation. This directory contains comprehensive documentation for the C++ search engine implementation.

## 📚 Documentation Index

### 🚀 Getting Started
- **[README.md](../README.md)** - Main project overview and quick start guide
- **[LICENSE](../LICENSE)** - Project license information

### 🔧 Development Documentation

#### JavaScript Minification & Caching
- **[PERFORMANCE_OPTIMIZATIONS_SUMMARY.md](./PERFORMANCE_OPTIMIZATIONS_SUMMARY.md)** - Complete performance optimization summary
  - 99.6% faster JavaScript file serving
  - Redis-based caching implementation
  - Production-grade HTTP headers
  - Comprehensive monitoring and testing
- **[JS_MINIFIER_CLIENT_CHANGELOG.md](./JS_MINIFIER_CLIENT_CHANGELOG.md)** - Detailed changelog for JsMinifierClient improvements
  - Enhanced JSON parsing with robust escape sequence handling
  - Size-based method selection (JSON ≤100KB, File Upload >100KB)
  - Improved error handling and debugging output
  - Performance optimizations and bug fixes
- **[JS_MINIFICATION_CACHING_STRATEGY.md](./development/JS_MINIFICATION_CACHING_STRATEGY.md)** - Comprehensive caching strategy analysis
  - Redis vs File vs Memory caching comparison
  - Hybrid caching approach implementation
  - Performance benchmarks and recommendations
- **[JS_CACHING_BEST_PRACTICES.md](./guides/JS_CACHING_BEST_PRACTICES.md)** - Production caching best practices
  - Redis cache implementation guide
  - Cache monitoring and optimization
  - Performance testing and validation
- **[JS_CACHING_HEADERS_BEST_PRACTICES.md](./guides/JS_CACHING_HEADERS_BEST_PRACTICES.md)** - HTTP caching headers guide
  - Production-grade caching headers implementation
  - Browser cache optimization strategies
  - CDN integration and performance tuning

#### Project Organization
- **[DOCUMENTATION_CLEANUP.md](./DOCUMENTATION_CLEANUP.md)** - Documentation organization and cleanup guidelines

### 📁 Directory Structure

```
docs/
├── README.md                           # This documentation index
├── PERFORMANCE_OPTIMIZATIONS_SUMMARY.md # Complete performance optimization summary
├── JS_MINIFIER_CLIENT_CHANGELOG.md     # JsMinifierClient version history
├── DOCUMENTATION_CLEANUP.md            # Documentation organization guidelines
├── guides/                             # User and developer guides
│   ├── JS_CACHING_BEST_PRACTICES.md    # Production caching best practices
│   ├── JS_CACHING_HEADERS_BEST_PRACTICES.md # HTTP caching headers guide
│   ├── JS_MINIFICATION_STRATEGY_ANALYSIS.md # Implementation strategy analysis
│   └── README_JS_MINIFICATION.md       # JavaScript minification features
└── development/                        # Technical development docs
    └── JS_MINIFICATION_CACHING_STRATEGY.md # Comprehensive caching strategy
```

### 🎯 Quick Navigation

#### For Developers
- **New to the project?** Start with [../README.md](../README.md)
- **Working on JS minification?** See [JS_MINIFIER_CLIENT_CHANGELOG.md](./JS_MINIFIER_CLIENT_CHANGELOG.md)
- **Implementing caching?** See [JS_CACHING_BEST_PRACTICES.md](./guides/JS_CACHING_BEST_PRACTICES.md)
- **Optimizing headers?** See [JS_CACHING_HEADERS_BEST_PRACTICES.md](./guides/JS_CACHING_HEADERS_BEST_PRACTICES.md)
- **Contributing documentation?** Check [DOCUMENTATION_CLEANUP.md](./DOCUMENTATION_CLEANUP.md)

#### For Operations
- **Deployment guide** - See [../README.md](../README.md#deployment)
- **Configuration** - See [../config/](../config/) directory
- **Docker setup** - See [../docker/](../docker/) directory

### 🔍 Search Engine Components

#### Core Components
- **Search Engine** - C++20 implementation with RedisSearch integration
- **Web Crawler** - Multi-threaded web crawler with content parsing
- **Storage Layer** - MongoDB and Redis storage backends
- **API Server** - uWebSockets-based HTTP/WebSocket server

#### Microservices
- **JS Minifier** - Node.js microservice for JavaScript minification
- **Browserless** - Headless Chrome for dynamic content rendering
- **MongoDB** - Document database for content storage
- **Redis** - In-memory database for search indexing

### 📊 Architecture Overview

```
┌─────────────────┐    HTTP/JSON    ┌──────────────────┐
│   C++ Search    │ ──────────────► │  JS Minifier     │
│    Engine       │                 │   Microservice   │
│                 │ ◄────────────── │   (Node.js)      │
└─────────────────┘    Response     └──────────────────┘
         │
         │ WebSocket
         ▼
┌─────────────────┐    HTTP/JSON    ┌──────────────────┐
│   Web Crawler   │ ──────────────► │   Browserless    │
│                 │                 │   (Chrome)       │
└─────────────────┘                 └──────────────────┘
         │
         │ Storage
         ▼
┌─────────────────┐    ┌──────────────────┐
│     MongoDB     │    │      Redis       │
│   (Content)     │    │   (Search Index) │
└─────────────────┘    └──────────────────┘
```

### 🛠️ Development Workflow

#### Building the Project
```bash
# Build with Docker
docker-compose -f docker/docker-compose.yml up --build

# Build locally
./build.sh
```

#### Running Tests
```bash
# Run all tests
ctest --test-dir build --output-on-failure

# Run specific test categories
ctest -L "search_core"
ctest -L "integration"
```

#### Development Tools
- **Code Formatting** - Prettier configuration in [../.prettierrc.json](../.prettierrc.json)
- **Git Hooks** - See [../.github/](../.github/) directory
- **VS Code** - Configuration in [../.vscode/](../.vscode/) directory

### 📈 Performance & Monitoring

#### Key Metrics
- **Search Latency** - Target <5ms p95 for local Redis operations
- **Crawl Throughput** - Configurable via domain managers
- **Memory Usage** - Optimized for large-scale crawling
- **Storage Efficiency** - Compressed content storage

#### Monitoring
- **WebSocket Logs** - Real-time crawl progress
- **Health Checks** - Service availability monitoring
- **Performance Metrics** - Built-in timing and statistics

### 🔒 Security & Best Practices

#### Security Features
- **CSP Headers** - Content Security Policy implementation
- **Input Validation** - Comprehensive URL and content sanitization
- **Rate Limiting** - Configurable crawl rate limits
- **Error Handling** - Graceful failure recovery

#### Development Best Practices
- **Code Quality** - C++20 standards with comprehensive testing
- **Documentation** - Inline comments and detailed changelogs
- **Error Handling** - Robust error recovery and logging
- **Performance** - Optimized algorithms and data structures

### 🤝 Contributing

#### Documentation Standards
- **Markdown Format** - Use standard markdown with proper headings
- **Code Examples** - Include working code snippets
- **Screenshots** - Add visual aids where helpful
- **Version History** - Maintain detailed changelogs

#### Code Standards
- **C++20** - Use modern C++ features
- **Testing** - Comprehensive unit and integration tests
- **Documentation** - Clear inline comments and API docs
- **Performance** - Optimize for speed and memory efficiency

### 📞 Support & Resources

#### Getting Help
- **Issues** - Report bugs via GitHub issues
- **Discussions** - Join project discussions
- **Documentation** - Check this directory for guides
- **Examples** - See [../examples/](../examples/) directory

#### External Resources
- **uWebSockets** - [https://github.com/uNetworking/uWebSockets](https://github.com/uNetworking/uWebSockets)
- **RedisSearch** - [https://redis.io/docs/stack/search/](https://redis.io/docs/stack/search/)
- **MongoDB** - [https://docs.mongodb.com/](https://docs.mongodb.com/)
- **Terser** - [https://terser.org/](https://terser.org/)

---

**Last Updated**: June 2024  
**Version**: 2.0  
**Maintainer**: Search Engine Core Team
