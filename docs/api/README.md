# Search API Documentation

This directory contains the REST API documentation for the search engine with
enhanced crawler capabilities.

## Files

- **search_endpoint.md** - REST API specification for the `/search` endpoint
- **crawler_endpoint.md** - Comprehensive crawler API documentation with SPA
  rendering support
- **search_response.schema.json** - JSON Schema definition for the search
  response format
- **examples/** - Directory containing example JSON responses for testing

## API Endpoints Overview

### Search API

- `GET /api/search` - Full-text search functionality with Redis backend
- Support for pagination, domain filtering, and advanced query syntax
- See: [search_endpoint.md](./search_endpoint.md)

### Enhanced Crawler API

- `POST /api/crawl/add-site` - Add sites to crawl queue with SPA rendering
  support
- `GET /api/crawl/status` - Get overall crawling statistics and status
- `GET /api/crawl/details` - Get detailed crawl results for specific
  URLs/domains
- `POST /api/spa/render` - Direct SPA rendering without crawling
- `POST /api/spa/detect` - Analyze URLs for SPA characteristics
- See: [crawler_endpoint.md](./crawler_endpoint.md)

## Key Features

### 🚀 **Advanced SPA Rendering**

- **Intelligent detection** of React, Vue, Angular, and other JavaScript
  frameworks
- **Headless browser rendering** using browserless/Chrome for dynamic content
- **Title extraction** from JavaScript-rendered pages (e.g., Persian sites like
  digikala.com)
- **74x content improvement** over static HTML crawling (7KB → 580KB)

### 🎯 **Flexible Content Storage**

- **Preview mode**: 500-character snippets for lightweight indexing
- **Full content mode**: Complete rendered HTML for comprehensive search
- **Configurable storage** based on use case requirements

### ⚡ **Performance Optimizations**

- **30-second default timeout** optimized for complex JavaScript sites
- **Selective rendering** - only processes detected SPAs
- **Graceful fallback** to static HTML when rendering fails
- **Connection pooling** to browserless service

## Real-World Results

### Digikala.com Success Case

**Before SPA Rendering:**

- Title: ❌ Empty
- Content: ~7KB static HTML
- Status: Limited indexability

**After SPA Rendering:**

- Title: ✅ "فروشگاه اینترنتی دیجی‌کالا" (Persian)
- Content: ~580KB fully rendered content
- Status: Complete search indexability

## Validation

To validate example JSON files against the schema:

```bash
npm run validate-schema
```

To check formatting:

```bash
npm run format:check
```

To automatically fix formatting:

```bash
npm run format
```

## Quick Start Examples

### 1. Search with Domain Filter

```bash
curl "http://localhost:3000/api/search?q=machine%20learning&domain_filter=arxiv.org"
```

### 2. Crawl JavaScript-Heavy Site

```bash
curl -X POST http://localhost:3000/api/crawl/add-site \
  -H "Content-Type: application/json" \
  -d '{
    "url": "https://www.digikala.com",
    "spaRenderingEnabled": true,
    "includeFullContent": true
  }'
```

### 3. Direct SPA Rendering

```bash
curl -X POST http://localhost:3000/api/spa/render \
  -H "Content-Type: application/json" \
  -d '{
    "url": "https://www.digikala.com",
    "timeout": 60000,
    "includeFullContent": true
  }'
```

### 4. Check Crawl Results

```bash
curl "http://localhost:3000/api/crawl/details?url=https://www.digikala.com" | jq '.logs[0].title'
```

Expected: `"فروشگاه اینترنتی دیجی‌کالا"`

## CI/CD

The GitHub Actions workflow at `.github/workflows/api-validation.yml`
automatically:

- Validates all JSON examples against the schema
- Checks code formatting with Prettier
- Verifies required API documentation files exist
- Tests SPA rendering integration

This ensures the API documentation remains consistent and valid with the latest
crawler enhancements.
