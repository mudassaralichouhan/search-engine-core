# Crawler API Specification

## Overview

The crawler API provides advanced web crawling functionality with intelligent SPA detection and headless browser rendering capabilities for JavaScript-heavy websites.

## Endpoints

### `/api/crawl/add-site` - Add Site to Crawl Queue

Adds a website to the crawling queue with comprehensive configuration options including SPA rendering support.

**Method:** `POST`  
**Content-Type:** `application/json`

#### Request Parameters

| Parameter | Type | Default | Required | Description |
|-----------|------|---------|----------|-------------|
| `url` | string | - | ✅ | Target URL to crawl |
| `maxPages` | integer | 1000 | ❌ | Maximum number of pages to crawl (1-10000) |
| `maxDepth` | integer | 3 | ❌ | Maximum crawl depth (1-10) |
| `spaRenderingEnabled` | boolean | true | ❌ | Enable SPA detection and headless browser rendering |
| `includeFullContent` | boolean | false | ❌ | Store full content vs preview (like SPA render API) |
| `browserlessUrl` | string | "http://browserless:3000" | ❌ | Browserless service URL for SPA rendering |
| `restrictToSeedDomain` | boolean | true | ❌ | Limit crawling to the seed domain |
| `followRedirects` | boolean | true | ❌ | Follow HTTP redirects |
| `maxRedirects` | integer | 10 | ❌ | Maximum number of redirects to follow (0-20) |
| `force` | boolean | false | ❌ | Force re-crawl even if already crawled |

#### Example Request

```json
POST /api/crawl/add-site
{
  "url": "https://www.digikala.com",
  "maxPages": 100,
  "maxDepth": 2,
  "spaRenderingEnabled": true,
  "includeFullContent": true,
  "browserlessUrl": "http://browserless:3000",
  "restrictToSeedDomain": true,
  "followRedirects": true,
  "maxRedirects": 10
}
```

#### Success Response (200 OK)

```json
{
  "success": true,
  "message": "Site added to crawl queue successfully",
  "data": {
    "url": "https://www.digikala.com",
    "maxPages": 100,
    "maxDepth": 2,
    "restrictToSeedDomain": true,
    "followRedirects": true,
    "maxRedirects": 10,
    "spaRenderingEnabled": true,
    "includeFullContent": true,
    "browserlessUrl": "http://browserless:3000",
    "status": "queued"
  }
}
```

#### Error Responses

**400 Bad Request** - Invalid parameters:
```json
{
  "error": "URL is required",
  "success": false
}
```

**500 Internal Server Error** - Server error:
```json
{
  "error": "Crawler not initialized",
  "success": false
}
```

---

### `/api/crawl/status` - Get Crawl Status

Returns overall crawling statistics and current status.

**Method:** `GET`

#### Success Response (200 OK)

```json
{
  "statistics": {
    "failedCrawls": 0,
    "successRate": 100,
    "successfulCrawls": 15,
    "totalLinksFound": 247
  },
  "status": {
    "isRunning": true,
    "lastUpdate": 1753444512,
    "totalCrawled": 15
  }
}
```

---

### `/api/crawl/details` - Get Detailed Crawl Results

Returns detailed crawl results for a specific URL or domain.

**Method:** `GET`

#### Query Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `url` | string | ❌ | Specific URL to get details for |
| `domain` | string | ❌ | Domain to get all results for |

**Note:** Either `url` or `domain` parameter is required.

#### Example Requests

```bash
# Get details for specific URL
GET /api/crawl/details?url=https://www.digikala.com

# Get all results for domain
GET /api/crawl/details?domain=digikala.com
```

#### Success Response (200 OK)

```json
{
  "url": "https://www.digikala.com",
  "logs": [
    {
      "contentSize": 583940,
      "contentType": "text/html; charset=UTF-8",
      "crawlTime": 1753444482,
      "downloadTimeMs": 2783,
      "description": "فروشگاه اینترنتی دیجی‌کالا",
      "domain": "www.digikala.com",
      "httpStatusCode": 200,
      "id": "68837083d12f118e6d03eca3",
      "links": [],
      "status": "SUCCESS",
      "title": "فروشگاه اینترنتی دیجی‌کالا",
      "url": "https://www.digikala.com",
      "renderingMethod": "headless_browser",
      "spaDetected": true
    }
  ]
}
```

Note:
- downloadTimeMs: Total time in milliseconds to download the page (measured from download start to completion). This is recorded for every successfully processed URL.

#### Error Responses

**400 Bad Request** - Missing parameters:
```json
{
  "message": "Please provide a 'domain' or 'url' query parameter to fetch crawl details."
}
```

---

### `/api/spa/render` - Direct SPA Rendering

Renders a single page using headless browser without adding to crawl queue.

**Method:** `POST`  
**Content-Type:** `application/json`

#### Request Parameters

| Parameter | Type | Default | Required | Description |
|-----------|------|---------|----------|-------------|
| `url` | string | - | ✅ | URL to render |
| `timeout` | integer | 30000 | ❌ | Rendering timeout in milliseconds |
| `includeFullContent` | boolean | false | ❌ | Include full rendered HTML in response |

#### Example Request

```json
POST /api/spa/render
{
  "url": "https://www.digikala.com",
  "timeout": 60000,
  "includeFullContent": true
}
```

#### Success Response (200 OK)

```json
{
  "success": true,
  "url": "https://www.digikala.com",
  "fetchDuration": 28450,
  "httpStatusCode": 200,
  "contentType": "text/html; charset=UTF-8",
  "contentSize": 589000,
  "contentPreview": "<!DOCTYPE html><html lang=\"fa\" dir=\"rtl\">...",
  "isSpa": true,
  "renderingMethod": "headless_browser",
  "content": "<!-- Full rendered HTML when includeFullContent=true -->"
}
```

#### Error Responses

**400 Bad Request** - Invalid JSON or missing URL:
```json
{
  "error": "URL is required and must be a string",
  "success": false
}
```

**500 Internal Server Error** - Rendering failed:
```json
{
  "error": "Internal server error",
  "success": false
}
```

---

### `/api/spa/detect` - SPA Detection

Analyzes a URL to determine if it's a Single Page Application.

**Method:** `POST`  
**Content-Type:** `application/json`

#### Request Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `url` | string | ✅ | URL to analyze for SPA patterns |

#### Example Request

```json
POST /api/spa/detect
{
  "url": "https://www.digikala.com"
}
```

#### Success Response (200 OK)

```json
{
  "success": true,
  "url": "https://www.digikala.com",
  "isSpa": true,
  "detectionReasons": [
    "Framework detected: React",
    "High script-to-content ratio: 0.85",
    "State object found: __NEXT_DATA__"
  ],
  "framework": "Next.js/React",
  "confidence": 0.95
}
```

## SPA Rendering Features

### Intelligent SPA Detection

The crawler automatically detects SPAs using multiple indicators:

1. **Framework Patterns**: React, Vue, Angular, Ember, Svelte, Next.js
2. **DOM Attributes**: `data-reactroot`, `ng-*`, `v-*`, `__nuxt`
3. **Script Analysis**: High script-to-content ratios
4. **State Objects**: `window.__initial_state__`, `window.__data__`

### Content Processing Modes

#### Preview Mode (`includeFullContent: false`)
- Stores first 500 characters + "..." suffix
- Suitable for discovery and lightweight indexing
- ~500 bytes storage per page

#### Full Content Mode (`includeFullContent: true`)
- Stores complete rendered HTML
- Optimal for search indexing and content analysis
- ~500KB+ storage per SPA page (74x more content than static HTML)

### Performance Characteristics

| Metric | Static HTML | SPA Rendered | Improvement |
|--------|-------------|--------------|-------------|
| **Content Size** | ~7KB | ~580KB | **74x larger** |
| **Title Extraction** | Often empty | ✅ Dynamic titles | **Success** |
| **Render Time** | 1-2 seconds | 25-35 seconds | Expected for JS |
| **Success Rate** | 100% | 95% (with fallback) | High reliability |

## Configuration Options

### Environment Variables

| Variable | Description | Default |
|----------|-------------|---------|
| `BROWSERLESS_URL` | Browserless service URL | http://browserless:3000 |
| `SPA_RENDERING_ENABLED` | Global SPA rendering toggle | true |
| `DEFAULT_TIMEOUT` | Default rendering timeout (ms) | 30000 |

### Docker Compose Configuration

```yaml
services:
  search-engine:
    environment:
      - BROWSERLESS_URL=http://browserless:3000
      - SPA_RENDERING_ENABLED=true
      - DEFAULT_TIMEOUT=30000

  browserless:
    image: browserless/chrome:latest
    environment:
      - MAX_CONCURRENT_SESSIONS=10
      - PREBOOT_CHROME=true
      - CONNECTION_TIMEOUT=60000
```

## Best Practices

### 1. Timeout Configuration
- **Standard sites**: 30 seconds (default)
- **Complex SPAs**: 60+ seconds  
- **Heavy JavaScript sites**: 90+ seconds

### 2. Content Storage Strategy
- **Discovery crawling**: `includeFullContent: false`
- **Search indexing**: `includeFullContent: true`
- **Monitor storage**: 580KB vs 7KB per page

### 3. Error Handling
- Always implements graceful fallback to static HTML
- Detailed error logging for debugging
- Non-blocking operation when browserless unavailable

### 4. Performance Monitoring
- Track SPA detection accuracy
- Monitor rendering success rates
- Watch browserless resource usage

## Real-World Example

### Crawling Digikala.com (Persian E-commerce)

**Request:**
```bash
curl -X POST http://localhost:3000/api/crawl/add-site \
  -H "Content-Type: application/json" \
  -d '{
    "url": "https://www.digikala.com",
    "spaRenderingEnabled": true,
    "includeFullContent": true,
    "maxPages": 1
  }'
```

**Results after processing:**
```bash
curl "http://localhost:3000/api/crawl/details?url=https://www.digikala.com" | jq '.logs[0]'
```

**Expected output:**
```json
{
  "title": "فروشگاه اینترنتی دیجی‌کالا",
  "contentSize": 583940,
  "status": "SUCCESS",
  "httpStatusCode": 200,
  "renderingMethod": "headless_browser",
  "spaDetected": true,
  "domain": "www.digikala.com"
}
```

**Key Achievements:**
- ✅ **Persian title extracted**: "فروشگاه اینترنتی دیجی‌کالا" (Digikala Online Store)
- ✅ **74x content increase**: 7KB → 580KB rich content
- ✅ **Proper Unicode handling**: Persian text correctly processed
- ✅ **Search indexable**: Full content available for search queries

## Troubleshooting

### Common Issues

#### 1. Empty Titles Despite SPA Rendering
```bash
# Check if SPA detection worked
curl "http://localhost:3000/api/crawl/details?url=https://example.com" | jq '.logs[0].contentSize'

# If contentSize < 50000, SPA rendering likely failed
# Check browserless logs: docker logs browserless
```

#### 2. Timeout Errors
```bash
# Symptoms in logs:
[ERROR] CURL error: Timeout was reached
[WARN] Failed to render SPA: timeout, using original content

# Solutions:
# - Increase timeout in request
# - Check browserless container health
# - Verify network connectivity
```

#### 3. Browserless Connection Issues
```bash
# Test browserless directly
curl -X POST http://localhost:3001/content \
  -H "Content-Type: application/json" \
  -d '{"url": "https://example.com"}'

# Should return rendered HTML
```

### Debug Logging

Enable detailed logging to track SPA processing:

```bash
# Set log level to DEBUG
export LOG_LEVEL=DEBUG

# Key log messages to watch:
[INFO] SPA detected for URL: https://example.com. Fetching with headless browser...
[INFO] Successfully fetched SPA-rendered HTML for URL: https://example.com  
[INFO] Document indexed successfully: https://example.com (title: Page Title)
```

This comprehensive crawler API provides powerful SPA rendering capabilities that successfully extract titles and content from JavaScript-heavy websites, achieving up to 74x content improvement over traditional static HTML crawling. 