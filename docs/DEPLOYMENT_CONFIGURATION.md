# Deployment Configuration Guide

This guide covers all configuration options for deploying the search-engine-core application in different environments.

## 📋 Environment Variables Reference

### Core Configuration

| Variable | Default | Description | Required | Example |
|----------|---------|-------------|----------|---------|
| `PORT` | `3000` | Server listening port | No | `8080` |
| `LOG_LEVEL` | `info` | Logging verbosity level | No | `debug` |
| `MONGODB_URI` | `mongodb://localhost:27017` | MongoDB connection string | Yes | `mongodb://admin:pass@mongodb:27017` |
| `SEARCH_REDIS_URI` | `tcp://localhost:6379` | Redis connection string | Yes | `tcp://redis:6379` |
| `SEARCH_REDIS_POOL_SIZE` | `4` | Redis connection pool size | No | `16` |
| `SEARCH_INDEX_NAME` | `search_index` | RedisSearch index name | No | `prod_search_index` |

### Build-Time Configuration

| Variable | Default | Description | Required | Example |
|----------|---------|-------------|----------|---------|
| `BUILD_MODE` | `development` | Controls Redis CLI installation method for faster development builds | No | `production` |

### SPA Rendering Configuration

| Variable | Default | Description | Required | Example |
|----------|---------|-------------|----------|---------|
| `SPA_RENDERING_ENABLED` | `true` | Enable SPA detection and rendering | No | `false` |
| `SPA_RENDERING_TIMEOUT` | `60000` | SPA rendering timeout (ms) | No | `30000` |
| `BROWSERLESS_URL` | `http://browserless:3000` | Browserless service URL | No | `http://browserless:3000` |
| `DEFAULT_REQUEST_TIMEOUT` | `60000` | Default request timeout (ms) | No | `30000` |

### JavaScript Minification Configuration

| Variable | Default | Description | Required | Example |
|----------|---------|-------------|----------|---------|
| `MINIFY_JS` | `true` | Enable JavaScript minification | No | `false` |
| `MINIFY_JS_LEVEL` | `advanced` | Minification optimization level | No | `basic` |
| `JS_MINIFIER_SERVICE_URL` | `http://js-minifier:3002` | JS minifier service URL | No | `http://js-minifier:3002` |
| `JS_CACHE_ENABLED` | `true` | Enable JavaScript caching | No | `false` |
| `JS_CACHE_TYPE` | `redis` | Cache backend type | No | `memory` |
| `JS_CACHE_TTL` | `3600` | Cache TTL in seconds | No | `7200` |
| `JS_CACHE_REDIS_DB` | `1` | Redis database for cache | No | `2` |

## 🔧 Build Optimization (BUILD_MODE)

### Understanding BUILD_MODE

The `BUILD_MODE` argument controls how Redis CLI is installed in the Docker image:

| Mode | Redis CLI Source | Build Speed | Use Case |
|------|------------------|-------------|----------|
| `development` (default) | Copied from builder stage | 🚀 **Fast** (~2-3 min) | Development, daily iteration |
| `production` | Fresh package install | 🐌 **Slower** (~3-4 min) | Production, CI/CD pipelines |

#### Development Mode Benefits
- **~30-60 seconds faster builds**
- **Copies Redis CLI binary** directly from builder stage
- **No package download** required in runner stage
- **Maximal Docker layer caching**
- **Perfect for rapid development cycles**

#### Production Mode Benefits
- **Always fresh packages** with latest security updates
- **Guaranteed consistency** with production environment
- **No cached binaries** that might be outdated
- **CI/CD friendly** for automated deployments

### Using BUILD_MODE

```bash
# Development mode (fast, default)
docker build --build-arg BUILD_MODE=development -t search-engine-core:dev .

# Production mode (fresh packages)
docker build --build-arg BUILD_MODE=production -t search-engine-core:prod .

# Or use the Makefile commands
make build-dev    # Development build
make build-prod   # Production build
```

## 🚀 Quick Deployment Examples

### Development Environment

```bash
# Create .env file for development
cat > .env << EOF
# Core Configuration
PORT=3000
LOG_LEVEL=debug

# Database Configuration
MONGODB_URI=mongodb://admin:password123@mongodb:27017
SEARCH_REDIS_URI=tcp://redis:6379
SEARCH_REDIS_POOL_SIZE=4

# SPA Rendering (Development - faster)
SPA_RENDERING_ENABLED=true
SPA_RENDERING_TIMEOUT=30000
BROWSERLESS_URL=http://browserless:3000

# JavaScript Minification
MINIFY_JS=true
MINIFY_JS_LEVEL=basic
JS_CACHE_ENABLED=true
JS_CACHE_TYPE=redis
EOF

# 🚀 Fast development build (recommended)
make dev

# Or start services manually (uses development mode by default)
docker-compose up -d
```

**Note:** The development environment automatically uses `BUILD_MODE=development` for faster builds. Use `make prod` for production deployments with fresh packages.

### Production Environment

```bash
# Create .env file for production
cat > .env << EOF
# Core Configuration
PORT=80
LOG_LEVEL=info

# Database Configuration
MONGODB_URI=mongodb://prod-user:secure-password@mongodb-prod:27017
SEARCH_REDIS_URI=tcp://redis-prod:6379
SEARCH_REDIS_POOL_SIZE=16
SEARCH_INDEX_NAME=prod_search_index

# SPA Rendering (Production - optimized)
SPA_RENDERING_ENABLED=true
SPA_RENDERING_TIMEOUT=60000
BROWSERLESS_URL=http://browserless-prod:3000

# JavaScript Minification (Production - optimized)
MINIFY_JS=true
MINIFY_JS_LEVEL=advanced
JS_CACHE_ENABLED=true
JS_CACHE_TYPE=redis
JS_CACHE_TTL=7200
JS_CACHE_REDIS_DB=1
EOF

# 🏭 Production build with fresh packages (recommended)
make prod

# Or start production services manually
docker-compose -f docker-compose.yml -f docker-compose.prod.yml up -d
```

### High-Performance Environment

```bash
# Create .env file for high-performance setup
cat > .env << EOF
# Core Configuration
PORT=80
LOG_LEVEL=warning

# Database Configuration (High-performance)
MONGODB_URI=mongodb://prod-user:secure-password@mongodb-cluster:27017
SEARCH_REDIS_URI=tcp://redis-cluster:6379
SEARCH_REDIS_POOL_SIZE=32
SEARCH_INDEX_NAME=high_perf_search

# SPA Rendering (Optimized)
SPA_RENDERING_ENABLED=true
SPA_RENDERING_TIMEOUT=45000
BROWSERLESS_URL=http://browserless-cluster:3000

# JavaScript Minification (Maximum optimization)
MINIFY_JS=true
MINIFY_JS_LEVEL=advanced
JS_CACHE_ENABLED=true
JS_CACHE_TYPE=redis
JS_CACHE_TTL=86400
JS_CACHE_REDIS_DB=1
EOF

# Deploy with resource optimization
docker-compose -f docker/docker-compose.prod.yml up -d
```

## 🔧 Configuration Tuning

### Log Level Optimization

Choose the appropriate log level based on your environment:

```bash
# Development - Maximum visibility
LOG_LEVEL=debug

# Staging - Balanced visibility
LOG_LEVEL=info

# Production - Essential information only
LOG_LEVEL=warning

# Critical monitoring - Errors only
LOG_LEVEL=error

# Performance testing - No logging overhead
LOG_LEVEL=none
```

### Database Connection Pool Sizing

Adjust connection pool sizes based on your load:

```bash
# Small application
SEARCH_REDIS_POOL_SIZE=4

# Medium application
SEARCH_REDIS_POOL_SIZE=16

# Large application
SEARCH_REDIS_POOL_SIZE=32

# Enterprise application
SEARCH_REDIS_POOL_SIZE=64
```

### SPA Rendering Optimization

Tune SPA rendering for your use case:

```bash
# Fast development iteration
SPA_RENDERING_TIMEOUT=30000

# Balanced performance
SPA_RENDERING_TIMEOUT=45000

# Complex SPA applications
SPA_RENDERING_TIMEOUT=60000

# Disable for static sites
SPA_RENDERING_ENABLED=false
```

## 🐳 Docker Compose Configuration

### Development (docker-compose.yml)

```yaml
services:
  search-engine:
    environment:
      - LOG_LEVEL=debug
      - PORT=3000
      - MONGODB_URI=mongodb://admin:password123@mongodb:27017
      - SEARCH_REDIS_URI=tcp://redis:6379
      - SPA_RENDERING_ENABLED=true
      - BROWSERLESS_URL=http://browserless:3000
```

### Production (docker/docker-compose.prod.yml)

```yaml
services:
  search-engine:
    environment:
      - LOG_LEVEL=info
      - PORT=80
      - MONGODB_URI=${MONGODB_URI}
      - SEARCH_REDIS_URI=${SEARCH_REDIS_URI}
      - SPA_RENDERING_ENABLED=true
      - BROWSERLESS_URL=${BROWSERLESS_URL}
```

## 📊 Monitoring and Observability

### Health Checks

The application includes comprehensive health checks:

```yaml
healthcheck:
  test: ["CMD-SHELL", "curl -f http://localhost:3000/health || exit 1"]
  interval: 30s
  timeout: 10s
  retries: 3
  start_period: 30s
```

### Log Monitoring

Monitor logs based on your LOG_LEVEL:

```bash
# Monitor all logs
docker-compose logs -f search-engine

# Monitor errors only
docker-compose logs -f search-engine | grep -i error

# Monitor WebSocket activity
docker-compose logs -f search-engine | grep -i websocket

# Monitor crawler activity
docker-compose logs -f search-engine | grep -i crawl
```

## 🔒 Security Considerations

### Environment Variable Security

1. **Never commit .env files** to version control
2. **Use strong passwords** for database credentials
3. **Limit database network exposure** in production
4. **Regularly rotate credentials**

### Network Security

```yaml
# Example: Expose only necessary ports
services:
  search-engine:
    ports:
      - "80:3000"  # HTTP only, no direct database access
    networks:
      - internal   # Isolated network

  mongodb:
    # No external port mapping for security
    networks:
      - internal
```

## 🚀 Scaling Configuration

### Horizontal Scaling

```yaml
# Scale application instances
docker-compose up -d --scale search-engine=3

# Scale browserless instances
docker-compose up -d --scale browserless=5
```

### Resource Optimization

```yaml
deploy:
  resources:
    limits:
      memory: 2G
      cpus: '2.0'
    reservations:
      memory: 1G
      cpus: '0.5'
```

## 🔧 Troubleshooting

### Common Configuration Issues

1. **Port conflicts**: Change `PORT` variable
2. **Database connection failures**: Verify `MONGODB_URI` and `SEARCH_REDIS_URI`
3. **SPA rendering failures**: Check `BROWSERLESS_URL` and timeouts
4. **Performance issues**: Adjust connection pool sizes and timeouts
5. **Memory issues**: Monitor with `docker stats` and adjust resource limits

### Debug Configuration

Enable detailed logging for troubleshooting:

```bash
# Temporary debug logging
docker-compose exec search-engine env LOG_LEVEL=debug

# Restart with debug logging
LOG_LEVEL=debug docker-compose restart search-engine

# View debug logs
docker-compose logs -f search-engine
```

---

## 📚 Additional Resources

- [Development Setup Guide](development/DEVELOPMENT_SETUP.md)
- [API Documentation](api/README.md)
- [Performance Optimization](PERFORMANCE_OPTIMIZATIONS_SUMMARY.md)
- [Docker Health Check Best Practices](DOCKER_HEALTH_CHECK_BEST_PRACTICES.md)
