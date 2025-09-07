# Development Setup Guide

This guide covers setting up the search-engine-core project for development, including debug configuration, testing, and best practices.

## 🚀 Quick Development Setup

### Prerequisites

- Docker and Docker Compose
- Git
- Basic understanding of C++, Node.js, and web development

### 1. Clone and Setup

```bash
# Clone the repository
git clone https://github.com/hatefsystems/search-engine-core.git
cd search-engine-core

# 🚀 Fast development setup (recommended)
make dev

# Or manually with debug logging
LOG_LEVEL=debug docker-compose up -d

# Check that services are running
docker-compose ps
```

#### **Development Build Optimization**

The project includes **optimized build modes** to speed up development:

**Fast Development Builds:**
```bash
# Uses BUILD_MODE=development (default)
make dev          # Start full environment
make build-dev    # Build image only

# Manual equivalent
docker build --build-arg BUILD_MODE=development -t search-engine-core:dev .
```

**Benefits:**
- ✅ **~30-60 seconds faster** builds
- ✅ **Copies Redis CLI** from builder stage (no package download)
- ✅ **Maximal Docker layer caching**
- ✅ **Perfect for rapid development iteration**

**When to use Production Builds:**
```bash
# For production deployment or fresh packages
make prod         # Start production environment
make build-prod   # Build production image only

# Manual equivalent
docker build --build-arg BUILD_MODE=production -t search-engine-core:prod .
```

### 2. Verify Setup

```bash
# Check service logs
docker-compose logs -f search-engine

# Test basic functionality
curl http://localhost:3000/api/search?q=test
```

## 🔧 Debug Configuration

### Environment Variables

The application uses the `LOG_LEVEL` environment variable to control logging verbosity:

| Log Level | Use Case | Example Output |
|-----------|----------|----------------|
| `trace` | Detailed execution flow | Function entry/exit, variable values |
| `debug` | Development troubleshooting | WebSocket connections, API calls, crawler progress |
| `info` | Standard development | System status, successful operations |
| `warning` | Production monitoring | Non-critical issues, performance warnings |
| `error` | Critical monitoring | System failures, database errors |
| `none` | Performance testing | No logging output |

### Development Debug Setup

```bash
# Maximum debug output for development
LOG_LEVEL=debug docker-compose up

# Include detailed execution tracing
LOG_LEVEL=trace docker-compose up

# Minimal output (focus on errors only)
LOG_LEVEL=error docker-compose up
```

### Runtime Log Level Changes

You can change log levels without restarting:

```bash
# Update environment variable
export LOG_LEVEL=debug

# Restart the application service
docker-compose restart search-engine

# Or use docker exec to change it dynamically
docker exec search-engine-core env LOG_LEVEL=trace
```

## 🧪 Testing

### Running Tests

```bash
# Run all tests
docker-compose exec search-engine ./tests/run_all_tests

# Run specific test suite
docker-compose exec search-engine ./tests/crawler/crawler_tests

# Run tests with debug logging
LOG_LEVEL=debug docker-compose exec search-engine ./tests/crawler/crawler_tests
```

### SPA Rendering Tests

```bash
# Test SPA detection and rendering
docker-compose exec search-engine ./tests/crawler/spa_tests

# Test with real websites
curl -X POST http://localhost:3000/api/spa/render \
  -H "Content-Type: application/json" \
  -d '{"url": "https://www.digikala.com"}'
```

### Performance Testing

```bash
# Test with minimal logging for performance measurement
LOG_LEVEL=none docker-compose up -d

# Run performance benchmarks
docker-compose exec search-engine ./tests/performance_benchmarks
```

## 🔍 Debugging Techniques

### WebSocket Debugging

```bash
# Monitor WebSocket connections
docker-compose logs -f search-engine | grep -i websocket

# Test WebSocket endpoint
curl -N -H "Connection: Upgrade" \
     -H "Upgrade: websocket" \
     -H "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==" \
     -H "Sec-WebSocket-Version: 13" \
     ws://localhost:3000/crawl-logs
```

### Crawler Debugging

```bash
# Monitor crawler activity
docker-compose logs -f search-engine | grep -i crawl

# Start crawl with debug logging
curl -X POST http://localhost:3000/api/crawl/add-site \
  -H "Content-Type: application/json" \
  -d '{
    "url": "https://example.com",
    "force": true,
    "extractTextContent": true,
    "spaRenderingEnabled": true
  }'
```

### Database Debugging

```bash
# Connect to MongoDB
docker-compose exec mongodb mongosh

# Check Redis data
docker-compose exec redis redis-cli

# Monitor database operations
docker-compose logs -f mongodb redis
```

## 🏗️ Development Workflow

### Code Changes

1. **Make changes** to source code
2. **Rebuild** the application:
   ```bash
   docker-compose build search-engine
   docker-compose up -d search-engine
   ```
3. **Test changes** with appropriate log level
4. **Run tests** to ensure functionality

### Adding New Features

1. **Plan the feature** and update relevant documentation
2. **Implement with debug logging** for easy troubleshooting
3. **Add comprehensive tests**
4. **Update documentation** including environment variables if added

### Performance Optimization

1. **Profile with debug logging** to identify bottlenecks
2. **Use LOG_LEVEL=trace** for detailed execution flow
3. **Test optimizations** with LOG_LEVEL=none for clean performance measurements
4. **Document performance improvements**

## 🚨 Troubleshooting

### Common Issues

#### Service Won't Start
```bash
# Check service logs
docker-compose logs search-engine

# Verify environment variables
docker-compose exec search-engine env | grep LOG_LEVEL

# Check resource usage
docker stats
```

#### Database Connection Issues
```bash
# Verify MongoDB is running
docker-compose ps mongodb

# Check MongoDB logs
docker-compose logs mongodb

# Test connection
docker-compose exec mongodb mongosh --eval "db.adminCommand('ping')"
```

#### High Memory Usage
```bash
# Monitor resource usage
docker stats

# Adjust resource limits in docker-compose.yml
# Reduce concurrent sessions in browserless service
# Use LOG_LEVEL=warning to reduce log volume
```

### Debug Commands

```bash
# View all environment variables
docker-compose exec search-engine env

# Check application configuration
docker-compose exec search-engine cat /app/config.json

# Monitor system resources
docker-compose exec search-engine top

# View detailed logs
docker-compose logs --tail=100 -f search-engine
```

## 📚 Additional Resources

- [API Documentation](../api/README.md)
- [MongoDB C++ Guide](MONGODB_CPP_GUIDE.md)
- [SPA Rendering Guide](../SPA_RENDERING.md)
- [Performance Optimization](../PERFORMANCE_OPTIMIZATIONS_SUMMARY.md)

## 🤝 Contributing

1. Follow the established debug logging patterns
2. Add appropriate LOG_DEBUG statements for new features
3. Update this guide when adding new debug features
4. Test with different log levels before submitting PRs

---

**Remember**: Use `LOG_LEVEL=debug` for development and `LOG_LEVEL=info` for production deployments.
