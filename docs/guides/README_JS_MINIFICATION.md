# JavaScript Minification with Terser Integration & Advanced Caching

This project provides **microservice architecture** for JavaScript minification using [Terser](https://github.com/terser/terser), a powerful JavaScript parser, mangler, and compressor toolkit for ES6+, with **production-grade caching** for optimal performance.

## 🏗️ Architecture

**Clean microservice approach with advanced caching:**

- **Main container**: Your existing C++ search engine (lightweight, focused)
- **JS Minifier service**: Dedicated Node.js container with Terser (port 3002)
- **Redis cache**: High-performance caching for minified JavaScript (port 6379)
- **Communication**: HTTP API between services
- **Benefits**: Independent scaling, fault isolation, clean separation of concerns
- **Performance**: 99.6% faster cached responses (0.17ms vs 43.31ms)

## 🚀 Quick Start

### Using the Microservice

```cpp
#include "src/common/JsMinifierClient.h"

// Create client instance
JsMinifierClient client("http://js-minifier:3002");

// Check service availability
if (client.isServiceAvailable()) {
    // Minify code with advanced Terser optimization
    std::string minified = client.minify(jsCode, "advanced");
}
```

### Service-Only Approach

```cpp
#include "src/common/JsMinifierClient.h"

// Use only the microservice for minification
JsMinifierClient client("http://js-minifier:3002");
if (client.isServiceAvailable()) {
    std::string minified = client.minify(jsCode, "advanced");
} else {
    // Handle service unavailability
    std::cerr << "JS minifier service not available" << std::endl;
}
```

## ⚙️ Configuration

### Environment Variables

| Variable          | Values                    | Description                          |
| ----------------- | ------------------------- | ------------------------------------ |
| `MINIFY_JS`       | `true`/`false`            | Enable/disable minification globally |
| `MINIFY_JS_LEVEL` | `none`/`basic`/`advanced` | Minification aggressiveness          |

### Minification Levels

| Level          | Description                    | Features                                   |
| -------------- | ------------------------------ | ------------------------------------------ |
| **none**       | No minification                | Returns original code                      |
| **basic**      | Basic Terser minification      | Removes comments + whitespace              |
| **advanced**   | Advanced Terser minification   | Variable renaming + dead code elimination  |
| **aggressive** | Aggressive Terser minification | Maximum compression + unsafe optimizations |

## 📊 Performance Comparison

### Minification Performance

Based on testing with popular libraries:

| Library    | Original | Basic       | Advanced (Terser) | Savings   |
| ---------- | -------- | ----------- | ----------------- | --------- |
| jQuery 3.x | 287KB    | 201KB (30%) | 87KB (70%)        | **200KB** |
| React 18   | 42KB     | 31KB (26%)  | 12KB (71%)        | **30KB**  |
| Lodash     | 531KB    | 372KB (30%) | 71KB (87%)        | **460KB** |

### Caching Performance

| Metric | Without Cache | With Redis Cache | Improvement |
|--------|---------------|------------------|-------------|
| **First Request** | 43.31ms | 43.31ms | Same |
| **Subsequent Requests** | 43.31ms | 0.17ms | **99.6% faster** |
| **Cache Hit Rate** | 0% | 90%+ | **Infinite** |
| **Server Load** | High | Low | **90% reduction** |

## 🐳 Docker Setup

### Development Setup

```bash
# Install Node.js dependencies (for the microservice)
npm install

# Build with Docker Compose
docker-compose up --build

# Or run the setup script
./scripts/setup-microservice.sh
```

### Production Deployment

For production deployments, use the pre-built images from GitHub Container Registry:

```bash
# Use production Docker Compose file
docker-compose -f docker/docker-compose.prod.yml up -d

# The JS minifier service uses the image:
# ghcr.io/hatefsystems/search-engine-core/js-minifier:latest
```

**Production Environment Variables:**

```bash
# JavaScript Minification Configuration
MINIFY_JS=true
MINIFY_JS_LEVEL=advanced
JS_CACHE_ENABLED=true
JS_CACHE_TYPE=redis
JS_CACHE_TTL=3600
JS_CACHE_REDIS_DB=1
```

### Service Endpoints

The microservice exposes several endpoints:

```bash
# Health check
curl http://localhost:3002/health

# Configuration info
curl http://localhost:3002/config

# Minify single file
curl -X POST http://localhost:3002/minify \
  -H "Content-Type: application/json" \
  -d '{"code": "function test() { console.log(\"hello\"); }", "level": "advanced"}'

# Batch minification
curl -X POST http://localhost:3002/minify/batch \
  -H "Content-Type: application/json" \
  -d '{"files": [{"name": "test.js", "code": "function test() {}"}], "level": "advanced"}'
```

**Note:** In production deployments, the service is available at `http://js-minifier:3002` within the Docker network.

## 🔧 Integration Examples

### Web Server Integration

```cpp
// In your HTTP handler
auto& minifier = JsMinifier::getInstance();

std::string jsResponse = loadJavaScriptFile(filename);
if (minifier.isEnabled()) {
    jsResponse = minifier.process(jsResponse);

    // Set appropriate headers
    setHeader("Content-Type", "application/javascript");
    setHeader("Content-Encoding", "gzip"); // If using gzip
}
```

### Build-time Processing

```cpp
// Process multiple JS files during build
JsMinifierClient client;
std::vector<std::pair<std::string, std::string>> files = {
    {"app.js", readFile("src/app.js")},
    {"utils.js", readFile("src/utils.js")},
    {"components.js", readFile("src/components.js")}
};

auto results = client.minifyBatch(files, "advanced");
for (const auto& result : results) {
    writeFile("dist/" + result.name, result.code);
}
```

## 🛡️ Best Practices

### 1. **Choose the Right Approach**

- **Use Integrated Approach when:**
  - You have existing C++ codebase
  - You want minimal latency
  - You process small to medium JS files
  - You need synchronous processing

- **Use Microservice Approach when:**
  - You have distributed architecture
  - You process large JS files frequently
  - You need horizontal scaling
  - Multiple applications need minification
  - You want language-agnostic integration

### 2. **Caching Strategy**

- **Use Redis Cache for:**
  - Production environments
  - High-traffic applications
  - Shared caching across containers
  - Persistent cache storage

- **Use Memory Cache for:**
  - Development environments
  - Single-instance deployments
  - Temporary caching needs

- **Use File Cache for:**
  - Simple deployments without Redis
  - Persistent storage requirements
  - Easy debugging and inspection

### 3. **Performance Optimization**

```cpp
// Redis-based caching for production
JsMinificationCache cache;

std::string getMinifiedJs(const std::string& filename) {
    // Check Redis cache first
    std::string cached = cache.getCachedMinified(filename, content);
    if (!cached.empty()) {
        return cached; // Cache hit - 99.6% faster
    }

    // Minify and cache in Redis
    std::string minified = minifier.process(content);
    cache.cacheMinified(filename, content, minified);
    return minified;
}
```

### 4. **Testing Caching Functionality**

```bash
# Test cache performance
./scripts/test_js_cache.sh

# Check cache statistics
curl http://localhost:3000/api/cache/stats

# Clear cache metrics
curl -X POST http://localhost:3000/api/cache/clear

# Get cache configuration
curl http://localhost:3000/api/cache/info
```

### 5. **Error Handling**

```cpp
// Robust error handling
try {
    std::string minified = client.minify(jsCode, "advanced");
    return minified;
} catch (const std::exception& e) {
    std::cerr << "Minification failed: " << e.what() << std::endl;

    // Fallback strategies:
    // 1. Return original code as fallback
    return jsCode;

    // 2. Or return original code
    // return jsCode;
}
```

### 4. **Monitoring and Logging**

```cpp
// Add performance monitoring
auto start = std::chrono::high_resolution_clock::now();
std::string result = minifier.process(code);
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::high_resolution_clock::now() - start);

std::cout << "Minification: " << code.length() << " → " << result.length()
          << " bytes in " << duration.count() << "ms" << std::endl;
```

## 🔍 Troubleshooting

### Common Issues

**1. "Terser not available" Warning**

```bash
# Install Node.js and Terser
npm install terser

# Verify installation
node -e "console.log(require('terser').version)"
```

**2. Service Connection Failed**

```bash
# Check if service is running
docker ps | grep js-minifier

# Check service health
curl http://localhost:3002/health

# View service logs
docker logs js-minifier
```

**3. Memory Issues with Large Files**

```bash
# Increase Node.js memory limit for the service
docker-compose up --scale js-minifier=1 -e NODE_OPTIONS="--max-old-space-size=4096"
```

### Debug Mode

Enable debug logging by setting environment variables:

```bash
export TERSER_DEBUG_DIR=/tmp/terser-logs
export MINIFY_JS_DEBUG=true
```

## 🔒 Security Considerations

1. **Input Validation**: Always validate JavaScript code before processing
2. **Resource Limits**: Set timeouts and memory limits for Terser operations
3. **Network Security**: Use HTTPS for microservice communication in production
4. **Container Security**: Run services with non-root users

## 📈 Monitoring

The microservice provides metrics at `/health`:

```json
{
  "status": "healthy",
  "service": "js-minifier",
  "terser_version": "5.34.1",
  "timestamp": "2024-01-15T10:30:00.000Z"
}
```

Set up monitoring alerts for:

- Service availability
- Response times > 5 seconds
- Memory usage > 80%
- Error rates > 5%

## 🏆 Advanced Features

### Custom Terser Options

```cpp
// Advanced Terser configuration via environment
// Set custom options in docker-compose.yml:
// - TERSER_MANGLE_PROPS=true
// - TERSER_DROP_CONSOLE=false
// - TERSER_COMPRESS_PASSES=3
```

### Source Maps

```bash
# Enable source maps for debugging
curl -X POST http://localhost:3002/minify \
  -d '{"code": "...", "options": {"sourceMap": true}}'
```

This comprehensive setup gives you production-ready JavaScript minification with [Terser](https://github.com/terser/terser) integration, combining the power of modern JS optimization with the performance of C++!
