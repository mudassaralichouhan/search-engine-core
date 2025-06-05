# Storage Layer Testing Guide

This guide covers building and testing the storage layer of the search engine, which includes MongoDB storage, RedisSearch integration, and unified content storage.

## Overview

The storage layer provides a dual-storage architecture:
- **MongoDB**: Structured metadata storage with rich querying capabilities
- **RedisSearch**: Full-text search indexing with real-time search performance
- **ContentStorage**: Unified interface coordinating both storage systems

## Prerequisites

### Required Software

1. **Visual Studio 2019/2022** with C++ development tools
2. **CMake 3.12+** (preferably latest version)
3. **vcpkg** package manager
4. **MongoDB Community Server** (for MongoDB tests)
5. **Redis with RediSearch module** (for Redis tests)

### MongoDB Setup

#### Windows Installation
```powershell
# Download and install MongoDB Community Server
# https://www.mongodb.com/try/download/community

# Start MongoDB service
net start mongodb

# Or run manually
mongod --dbpath "C:\data\db"
```

#### Verification
```powershell
# Test MongoDB connection
mongo --eval "db.runCommand({ping: 1})"
```

### Redis Setup

#### Windows Installation
```powershell
# Option 1: Redis for Windows (Microsoft Open Tech)
# Download from: https://github.com/microsoftarchive/redis/releases

# Option 2: Docker (recommended)
docker run -d -p 6379:6379 redislabs/redisearch:latest

# Option 3: WSL2 with Redis
wsl --install
# Then install Redis in WSL2
```

#### Verification
```powershell
# Test Redis connection
redis-cli ping
# Should return "PONG"

# Test RediSearch module
redis-cli "FT.INFO" "test_index"
```

## Build Scripts

### Batch Script (Windows CMD)

**File**: `build_and_test_storage.bat`

**Usage**:
```cmd
# Run all tests
build_and_test_storage.bat

# Clean build and run all tests
build_and_test_storage.bat clean

# Run specific test categories
build_and_test_storage.bat mongodb
build_and_test_storage.bat redis
build_and_test_storage.bat content

# Verbose output
build_and_test_storage.bat verbose
```

### PowerShell Script (Cross-platform)

**File**: `build_and_test_storage.ps1`

**Basic Usage**:
```powershell
# Run all tests
.\build_and_test_storage.ps1

# Run with parallel build
.\build_and_test_storage.ps1 all -Parallel

# Clean build with Release configuration
.\build_and_test_storage.ps1 clean -Configuration Release

# Skip build and run tests only
.\build_and_test_storage.ps1 mongodb -SkipBuild

# Get help
.\build_and_test_storage.ps1 help
```

**Advanced Options**:
```powershell
# Run with code coverage
.\build_and_test_storage.ps1 all -Coverage

# Skip dependency checks
.\build_and_test_storage.ps1 quick -SkipDependencyCheck

# Custom timeout
.\build_and_test_storage.ps1 content -Timeout 600
```

## Test Categories

### MongoDB Storage Tests (`test_mongodb_storage`)

Tests the MongoDB storage layer functionality:

- **Connection and Initialization**: Database connection, index creation
- **CRUD Operations**: Create, read, update, delete site profiles
- **Batch Operations**: Bulk inserts and queries
- **Error Handling**: Connection failures, validation errors
- **Data Integrity**: BSON conversion, data consistency

**Key Test Cases**:
- Site profile storage and retrieval
- Domain-based queries
- Crawl status filtering
- Index performance
- Connection resilience

### Redis Storage Tests (`test_redis_search_storage`)

Tests the RedisSearch integration:

- **Index Management**: Creation, deletion, schema validation
- **Document Indexing**: Adding, updating, removing documents
- **Search Operations**: Full-text search, filtering, ranking
- **Advanced Features**: Highlighting, suggestions, faceted search
- **Batch Operations**: Bulk indexing and deletion

**Key Test Cases**:
- Document indexing and search
- Search query parsing
- Result ranking and scoring
- Index statistics
- Memory management

### Content Storage Tests (`test_content_storage`)

Integration tests for the unified storage interface:

- **Crawl Result Processing**: Converting crawler output to storage format
- **Dual Storage Coordination**: MongoDB and Redis synchronization
- **Search Integration**: End-to-end search functionality
- **Batch Processing**: Multiple document handling
- **Statistics and Monitoring**: Health checks, performance metrics

**Key Test Cases**:
- CrawlResult to SiteProfile conversion
- Dual storage consistency
- Search result ranking
- Error recovery
- Performance monitoring

## Configuration

### Test Configuration File

**File**: `storage_test_config.json`

```json
{
  "mongodb": {
    "connection_string": "mongodb://localhost:27017",
    "database_name": "test-search-engine",
    "test_timeout_seconds": 30
  },
  "redis": {
    "connection_string": "tcp://127.0.0.1:6379",
    "index_name": "test_search_index",
    "test_timeout_seconds": 30
  }
}
```

### Environment Variables

Set these environment variables for custom configuration:

```powershell
$env:MONGODB_URI = "mongodb://localhost:27017"
$env:REDIS_URI = "tcp://127.0.0.1:6379"
$env:LOG_LEVEL = "DEBUG"  # TRACE, DEBUG, INFO, WARN, ERROR
```

## Troubleshooting

### Common Build Issues

#### 1. vcpkg Dependencies

**Problem**: Redis dependencies fail to build
```
Error: Failed to build redis-plus-plus, hiredis
```

**Solution**:
```powershell
# Update vcpkg
git -C vcpkg pull
.\vcpkg\bootstrap-vcpkg.bat

# Clean and reinstall
.\vcpkg\vcpkg remove redis-plus-plus hiredis
.\vcpkg\vcpkg install redis-plus-plus hiredis --triplet x64-windows
```

#### 2. MongoDB Driver Issues

**Problem**: MongoDB C++ driver not found
```
Error: Could not find mongocxx, bsoncxx
```

**Solution**:
```powershell
# Install MongoDB C++ driver via vcpkg
.\vcpkg\vcpkg install mongo-cxx-driver --triplet x64-windows

# Or download and build manually from:
# https://github.com/mongodb/mongo-cxx-driver
```

#### 3. CMake Configuration Failures

**Problem**: CMake cannot find dependencies
```
Error: Could not find package mongocxx
```

**Solution**:
```powershell
# Set vcpkg toolchain
cmake .. -DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake

# Or set environment variable
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
```

### Common Runtime Issues

#### 1. MongoDB Connection Failed

**Problem**: Tests skip with "MongoDB not available"

**Solutions**:
1. **Check MongoDB service**:
   ```powershell
   net start mongodb
   # or
   sc query mongodb
   ```

2. **Manual start**:
   ```powershell
   mongod --dbpath "C:\data\db"
   ```

3. **Connection string**:
   ```
   mongodb://localhost:27017  # Default
   mongodb://user:pass@host:port/db  # Authenticated
   ```

#### 2. Redis Connection Failed

**Problem**: Redis tests fail with connection errors

**Solutions**:
1. **Check Redis process**:
   ```powershell
   netstat -an | findstr 6379
   redis-cli ping
   ```

2. **Start Redis**:
   ```powershell
   redis-server
   # or
   docker run -d -p 6379:6379 redis:latest
   ```

3. **RediSearch module**:
   ```powershell
   # Check if RediSearch is loaded
   redis-cli MODULE LIST
   
   # Use Redis with RediSearch
   docker run -d -p 6379:6379 redislabs/redisearch:latest
   ```

#### 3. Permission Issues

**Problem**: Access denied errors

**Solutions**:
1. **Run as Administrator**: Right-click PowerShell → "Run as Administrator"
2. **File permissions**: Ensure build directory is writable
3. **Antivirus**: Add project directory to antivirus exclusions

### Test-Specific Issues

#### 1. Tests Skip or Timeout

**Problem**: Tests complete too quickly or timeout

**Solutions**:
- Increase timeout: `-Timeout 600`
- Check service availability
- Verify test data cleanup
- Monitor resource usage

#### 2. Memory Issues

**Problem**: Out of memory during large batch tests

**Solutions**:
- Reduce batch size in tests
- Increase available RAM
- Check for memory leaks
- Use Release build configuration

#### 3. Index Creation Failures

**Problem**: Search index creation fails

**Solutions**:
- Verify Redis memory settings
- Check RediSearch module availability
- Ensure proper cleanup between tests
- Monitor Redis memory usage

## Performance Optimization

### Build Performance

1. **Parallel Building**:
   ```powershell
   .\build_and_test_storage.ps1 all -Parallel
   ```

2. **Incremental Builds**:
   - Avoid `clean` unless necessary
   - Use existing CMake cache
   - Build specific targets only

3. **Release Configuration**:
   ```powershell
   .\build_and_test_storage.ps1 all -Configuration Release
   ```

### Test Performance

1. **Service Optimization**:
   - Use SSD storage for databases
   - Allocate sufficient RAM
   - Close unnecessary applications

2. **Test Parallelization**:
   - Enable parallel test execution
   - Use separate test databases
   - Avoid resource contention

3. **Targeted Testing**:
   ```powershell
   # Test specific components
   .\build_and_test_storage.ps1 mongodb
   .\build_and_test_storage.ps1 redis
   
   # Quick essential tests only
   .\build_and_test_storage.ps1 quick
   ```

## Continuous Integration

### GitHub Actions Example

```yaml
name: Storage Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: windows-latest
    services:
      mongodb:
        image: mongo:latest
        ports:
          - 27017:27017
      redis:
        image: redislabs/redisearch:latest
        ports:
          - 6379:6379
    steps:
      - uses: actions/checkout@v3
      - name: Setup vcpkg
        run: |
          git clone https://github.com/Microsoft/vcpkg.git
          .\vcpkg\bootstrap-vcpkg.bat
      - name: Run Storage Tests
        run: .\build_and_test_storage.ps1 all -SkipDependencyCheck
```

### Local Development Workflow

1. **Initial Setup**:
   ```powershell
   # One-time setup
   .\build_and_test_storage.ps1 clean
   ```

2. **Development Cycle**:
   ```powershell
   # Quick iteration
   .\build_and_test_storage.ps1 mongodb -SkipBuild
   
   # Full validation
   .\build_and_test_storage.ps1 all
   ```

3. **Pre-commit**:
   ```powershell
   # Comprehensive testing
   .\build_and_test_storage.ps1 all -Configuration Release -Parallel
   ```

## Monitoring and Logging

### Test Results

- **XML Report**: `build/test_results.xml` (JUnit format)
- **Console Output**: Real-time progress and results
- **Build Logs**: CMake and compiler output

### Performance Metrics

- **Build Time**: Component and test build duration
- **Test Execution Time**: Individual and total test runtime
- **Resource Usage**: Memory and CPU utilization

### Debug Information

Enable verbose logging:
```powershell
# PowerShell script
.\build_and_test_storage.ps1 verbose

# Environment variable
$env:LOG_LEVEL = "DEBUG"
```

## Support

### Getting Help

1. **Script Help**:
   ```powershell
   .\build_and_test_storage.ps1 help
   ```

2. **Documentation**: See `docs/content-storage-layer.md`

3. **Issues**: Check build logs and test output for specific error messages

### Common Solutions

- **Clean build**: Use `clean` parameter to start fresh
- **Update dependencies**: Refresh vcpkg and rebuild
- **Check services**: Verify MongoDB and Redis are running
- **Environment**: Ensure proper Visual Studio and CMake setup

This guide should help you successfully build and test the storage layer components. For additional support, refer to the main project documentation or create an issue with specific error details. 