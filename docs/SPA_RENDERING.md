# SPA Rendering with Browserless/Chrome

This document explains how to set up and use the Single Page Application (SPA)
rendering functionality in the search engine crawler with enhanced integration
and performance optimizations.

## Overview

Many modern websites (like digikala.com, React apps, Vue applications) use
client-side rendering (CSR) and are single-page applications (SPA). Traditional
crawlers only fetch the initial HTML skeleton, missing the dynamically loaded
content. This implementation uses browserless/chrome to render SPAs and extract
the fully rendered HTML with proper title extraction and content processing.

## Architecture

```
┌─────────────────┐    HTTP/JSON    ┌──────────────────┐
│   C++ Crawler   │ ──────────────► │  Browserless/    │
│                 │                 │  Chrome          │
│  PageFetcher    │                 │                  │
│  + SPA Detect   │                 │  Headless Chrome │
│  + Content Ext  │                 │  + JS Execution  │
│  + Title Extraction              │  + DOM Rendering │
└─────────────────┘                 └──────────────────┘
```

## Enhanced Features (Latest Updates)

### 🚀 **Intelligent SPA Detection**

- **Framework Detection**: React, Vue, Angular, Ember, Svelte, Next.js patterns
- **DOM Pattern Analysis**: `data-reactroot`, `ng-*`, `v-*`, `__nuxt` attributes
- **Content Analysis**: Script-heavy pages with minimal initial HTML
- **State Object Detection**: `window.__initial_state__`, `window.__data__`,
  `window.__NEXT_DATA__`

### 🎯 **Enhanced Content Processing**

- **Dynamic Title Extraction**: Successfully extracts titles from
  JavaScript-rendered content
- **Full Content Support**: `includeFullContent` parameter for complete HTML
  storage
- **Content Size Optimization**: Preview mode (500 chars) vs full content mode
- **Unicode Support**: Proper handling of Persian, Arabic, Chinese, and other
  Unicode content

### ⚡ **Performance Optimizations**

- **30-second default timeout** for complex JavaScript sites
- **Selective rendering** - only processes detected SPAs
- **Connection pooling** to browserless service
- **Graceful fallback** to static HTML if rendering fails

## Setup

### 1. Dependencies

Install the required C++ libraries:

```bash
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev nlohmann-json3-dev

# CentOS/RHEL
sudo yum install libcurl-devel nlohmann-json-devel

# Or build from source if not available in package manager
```

### 2. Enhanced Docker Compose (with Kafka Frontier)

The `docker-compose.yml` includes browserless for SPA rendering and
Kafka/Zookeeper for the durable crawl frontier:

```yaml
services:
  search-engine:
    build: .
    ports:
      - "3000:3000"
    environment:
      - MONGODB_URI=mongodb://mongodb:27017
      - REDIS_URI=tcp://redis:6379
      - KAFKA_BOOTSTRAP_SERVERS=kafka:9092
      - KAFKA_FRONTIER_TOPIC=crawl.frontier
      - BROWSERLESS_URL=http://browserless:3000
      - SPA_RENDERING_ENABLED=true
      - DEFAULT_TIMEOUT=30000
    depends_on:
      - mongodb
      - redis
      - browserless
      - kafka

  browserless:
    image: browserless/chrome:latest
    container_name: browserless
    ports:
      - "3001:3000"
    environment:
      - MAX_CONCURRENT_SESSIONS=10
      - PREBOOT_CHROME=true
      - CONNECTION_TIMEOUT=60000
      - CHROME_REFRESH_TIME=60000
    networks:
      - search-network
    restart: unless-stopped

  zookeeper:
    image: bitnami/zookeeper:3.9
    environment:
      - ALLOW_ANONYMOUS_LOGIN=yes
    ports:
      - "2181:2181"
    networks:
      - search-network

  kafka:
    image: bitnami/kafka:3.7
    depends_on:
      - zookeeper
    environment:
      - KAFKA_CFG_ZOOKEEPER_CONNECT=zookeeper:2181
      - KAFKA_CFG_LISTENERS=PLAINTEXT://:9092
      - KAFKA_CFG_ADVERTISED_LISTENERS=PLAINTEXT://kafka:9092
      - KAFKA_CFG_AUTO_CREATE_TOPICS_ENABLE=true
      - ALLOW_PLAINTEXT_LISTENER=yes
    ports:
      - "9092:9092"
    networks:
      - search-network

  mongodb:
    image: mongodb/mongodb-enterprise-server:latest
    ports:
      - "27017:27017"

  redis:
    image: redis:latest
    ports:
      - "6379:6379"
```

### 3. Build with SPA Support

```bash
# Build with enhanced SPA rendering support
cmake -DCMAKE_BUILD_TYPE=Release .
make -j$(nproc)
```

## Enhanced Usage

### 1. Crawler API with SPA Support

```cpp
#include "controllers/SearchController.h"

// Enhanced crawler API with SPA parameters
POST /api/crawl/add-site
{
  "url": "https://www.digikala.com",
  "maxPages": 100,
  "maxDepth": 2,
  "spaRenderingEnabled": true,
  "includeFullContent": true,
  "browserlessUrl": "http://browserless:3000"
}
```

**Response with SPA data:**

```json
{
  "success": true,
  "message": "Site added to crawl queue successfully",
  "data": {
    "url": "https://www.digikala.com",
    "spaRenderingEnabled": true,
    "includeFullContent": true,
    "status": "queued"
  }
}
```

### 2. Direct SPA Rendering API

```cpp
// Direct SPA rendering endpoint
POST /api/spa/render
{
  "url": "https://www.digikala.com",
  "timeout": 60000,
  "includeFullContent": true
}
```

**Enhanced Response:**

```json
{
  "success": true,
  "url": "https://www.digikala.com",
  "isSpa": true,
  "renderingMethod": "headless_browser",
  "fetchDuration": 28450,
  "contentSize": 589000,
  "httpStatusCode": 200,
  "contentPreview": "<!DOCTYPE html>...",
  "content": "<!-- Full rendered HTML when includeFullContent=true -->"
}
```

### 3. Programmatic Usage

```cpp
#include "search_engine/crawler/PageFetcher.h"

// Create PageFetcher with enhanced SPA support
PageFetcher fetcher(
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
    std::chrono::milliseconds(30000),  // Increased timeout
    true,  // follow redirects
    5      // max redirects
);

// Enable SPA rendering with custom browserless URL
fetcher.setSpaRendering(true, "http://browserless:3000");

// Fetch URL (automatically detects and renders SPAs)
auto result = fetcher.fetch("https://www.digikala.com");
if (result.success) {
    std::cout << "Content size: " << result.content.size() << " bytes" << std::endl;
    // Expected: ~580KB for SPA-rendered content vs ~7KB for static HTML
}
```

## Enhanced SPA Detection

### Framework Detection Patterns

The system detects SPAs using comprehensive pattern matching:

```cpp
// React/Next.js Detection
- "data-reactroot"
- "__NEXT_DATA__"
- "_app-"
- "react"

// Vue/Nuxt Detection
- "data-server-rendered"
- "__NUXT__"
- "v-"
- "vue"

// Angular Detection
- "ng-"
- "angular"
- "data-ng-"

// Other Frameworks
- "ember"
- "svelte"
- "backbone"
```

### Content Analysis

```cpp
bool PageFetcher::isSpaPage(const std::string& html, const std::string& url) {
    // Check for framework indicators
    if (containsFrameworkPatterns(html)) return true;

    // Analyze script-to-content ratio
    if (getScriptToContentRatio(html) > 0.3) return true;

    // Check for state objects
    if (containsStateObjects(html)) return true;

    // Domain-specific patterns
    if (isDomainKnownSpa(url)) return true;

    return false;
}
```

## Performance Characteristics

### Rendering Performance

| Site Type            | Static HTML | SPA Rendered                    | Improvement               |
| -------------------- | ----------- | ------------------------------- | ------------------------- |
| **Content Size**     | ~7KB        | ~580KB                          | **74x larger**            |
| **Title Extraction** | ❌ Empty    | ✅ "فروشگاه اینترنتی دیجی‌کالا" | **Success**               |
| **Render Time**      | 1-2s        | 25-35s                          | Expected for JS execution |
| **Success Rate**     | 100%        | 95% (with fallback)             | High reliability          |

### Timeout Configuration

```cpp
// Default timeouts (optimized for SPA rendering)
struct CrawlConfig {
    std::chrono::milliseconds requestTimeout{30000};  // 30 seconds
    bool spaRenderingEnabled = true;
    std::string browserlessUrl = "http://browserless:3000";
    bool includeFullContent = false;  // Configurable per request
};
```

### Resource Usage

- **Browserless sessions**: Limited to 10 concurrent sessions
- **Memory per session**: ~100-200MB per Chrome instance
- **CPU usage**: Moderate during JavaScript execution
- **Network**: ~500KB-2MB per SPA render (vs ~7KB static)

## Optimization Strategies

### 1. Selective Rendering

```cpp
// Only render detected SPAs
if (pageFetcher->isSpaPage(initialHtml, url)) {
    auto spaResult = pageFetcher->renderWithBrowserless(url);
    if (spaResult.success) {
        content = spaResult.content;  // ~580KB rich content
    } else {
        content = initialHtml;        // Fallback to ~7KB static
    }
}
```

### 2. Content Storage Optimization

```cpp
// Configurable content storage
if (config.includeFullContent) {
    result.rawContent = fetchResult.content;  // Full 580KB content
} else {
    std::string preview = fetchResult.content.substr(0, 500);
    if (fetchResult.content.size() > 500) preview += "...";
    result.rawContent = preview;  // 500-char preview
}
```

### 3. Connection Pooling

```cpp
// Efficient browserless connection management
class BrowserlessClient {
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
    void reuseConnection();
    bool isAvailable();
};
```

## Real-World Results

### Digikala.com Case Study

**Before SPA Rendering:**

```json
{
  "title": "",
  "contentSize": 7865,
  "status": "SUCCESS",
  "httpStatusCode": 200,
  "renderingMethod": "direct_fetch"
}
```

**After SPA Rendering:**

```json
{
  "title": "فروشگاه اینترنتی دیجی‌کالا",
  "contentSize": 583940,
  "status": "SUCCESS",
  "httpStatusCode": 200,
  "renderingMethod": "headless_browser",
  "fetchDuration": 28450
}
```

**Key Improvements:**

- ✅ **Title extracted**: Persian title properly captured
- ✅ **74x content increase**: 7KB → 580KB rich content
- ✅ **Search indexable**: Full content available for search
- ✅ **Unicode support**: Proper Persian text handling

## Troubleshooting

### Common Issues and Solutions

#### 1. **Timeout Errors**

```bash
# Symptoms
[ERROR] CURL error: Timeout was reached
[WARN] Failed to render SPA: timeout, using original content

# Solutions
- Increase timeout to 60+ seconds for heavy sites
- Check browserless container health
- Verify network connectivity
```

#### 2. **Title Not Extracted**

```bash
# Check if SPA detection worked
curl "http://localhost:3000/api/crawl/details?url=https://example.com" | jq '.logs[0] | {title, status, contentSize}'

# Expected for successful SPA rendering:
{
  "title": "Actual Site Title",
  "status": "SUCCESS",
  "contentSize": 500000+
}
```

#### 3. **Browserless Connection Issues**

```bash
# Test browserless directly
curl -X POST http://localhost:3001/content \
  -H "Content-Type: application/json" \
  -d '{"url": "https://example.com"}'

# Check browserless logs
docker logs browserless
```

### Debug Logging

Enable comprehensive debugging:

```cpp
// Enable debug logging
Logger::setLevel(LogLevel::DEBUG);

// Key log messages to watch for:
[INFO] SPA detected for URL: https://example.com. Fetching with headless browser...
[INFO] Successfully fetched SPA-rendered HTML for URL: https://example.com
[INFO] Document indexed successfully: https://example.com (title: Site Title)
```

### Health Monitoring

```bash
# Check service health
curl http://localhost:3000/api/crawl/status

# Monitor browserless sessions
curl http://localhost:3001/metrics

# Check content storage
curl "http://localhost:3000/api/crawl/details?url=https://example.com" | jq '.logs[0].contentSize'
```

## Integration Examples

### Complete SPA Crawling Workflow

```bash
# 1. Start services
docker-compose up -d

# 2. Crawl JavaScript-heavy site
curl -X POST http://localhost:3000/api/crawl/add-site \
  -H "Content-Type: application/json" \
  -d '{
    "url": "https://www.digikala.com",
    "spaRenderingEnabled": true,
    "includeFullContent": true,
    "maxPages": 1
  }'

# 3. Wait for processing (30-60 seconds)
sleep 60

# 4. Check results
curl "http://localhost:3000/api/crawl/details?url=https://www.digikala.com" | jq '.logs[0] | {title, contentSize, status}'

# Expected output:
{
  "title": "فروشگاه اینترنتی دیجی‌کالا",
  "contentSize": 583940,
  "status": "SUCCESS"
}
```

### Custom Framework Detection

```cpp
// Add custom SPA detection patterns
bool PageFetcher::isSpaPage(const std::string& html, const std::string& url) {
    // Custom framework patterns
    std::vector<std::string> customPatterns = {
        "data-custom-app",
        "my-framework",
        "custom-state-object"
    };

    for (const auto& pattern : customPatterns) {
        if (html.find(pattern) != std::string::npos) {
            return true;
        }
    }

    return standardSpaDetection(html, url);
}
```

## Best Practices

### 1. **Timeout Configuration**

- Use 30+ seconds for complex SPAs
- Start with 60 seconds for unknown sites
- Monitor actual render times and adjust

### 2. **Content Storage Strategy**

- Use `includeFullContent: false` for discovery crawling
- Enable `includeFullContent: true` for final indexing
- Monitor storage usage (580KB vs 7KB per page)

### 3. **Performance Monitoring**

- Track SPA detection accuracy
- Monitor browserless session usage
- Watch for timeout patterns by domain

### 4. **Error Handling**

- Always implement fallback to static HTML
- Log detailed error information
- Provide graceful degradation

## Future Enhancements

### Planned Features

1. **Advanced Detection**: Machine learning models for SPA detection
2. **Framework Optimization**: Specific rendering strategies per framework
3. **Performance Caching**: Cache rendered results for repeated crawls
4. **Distributed Rendering**: Scale browserless across multiple nodes
5. **Real-time Monitoring**: Live dashboards for SPA rendering metrics

### Performance Improvements

1. **Smart Caching**: Cache rendered content for duplicate URLs
2. **Parallel Processing**: Multiple concurrent SPA rendering sessions
3. **Selective Re-rendering**: Only re-render when content changes
4. **Resource Optimization**: Memory and CPU usage improvements
5. **Network Optimization**: Reduce bandwidth usage for large SPAs

This comprehensive SPA rendering system successfully transforms static HTML
crawling into dynamic content extraction, achieving 74x content improvement and
proper title extraction from JavaScript-heavy websites like www.digikala.com.
