# MongoDB C++ Development Guide

## Overview

This guide covers MongoDB C++ driver integration patterns and critical lessons learned from the search-engine-core project.

## Critical Lessons Learned

### 🚨 **MOST IMPORTANT: MongoDB Instance Initialization**

**❌ NEVER DO THIS - Will Crash Server:**
```cpp
class MyStorage {
    MyStorage() {
        // This will cause "Empty reply from server" crash!
        mongocxx::uri uri{"mongodb://localhost:27017"};
        client_ = std::make_unique<mongocxx::client>(uri); // ❌ CRASH!
    }
};
```

**✅ ALWAYS DO THIS - Use Singleton Pattern:**
```cpp
class MyStorage {
    MyStorage() {
        // 1. Get the global MongoDB instance first
        mongocxx::instance& instance = MongoDBInstance::getInstance();
        
        // 2. Now create client safely
        mongocxx::uri uri{"mongodb://localhost:27017"};
        client_ = std::make_unique<mongocxx::client>(uri); // ✅ WORKS!
    }
};
```

## Error Symptoms

### Server Crashes
- `curl: (52) Empty reply from server`
- Server crashes immediately on API call
- No error logs because crash happens before logging
- Application terminates without warning

### Root Cause
- MongoDB C++ driver allows only **ONE global instance** per application
- Multiple instances cause memory corruption and crashes
- Instance must be initialized before any client creation

## Proper Implementation Pattern

### 1. Header File (`include/mongodb.h`)

```cpp
#include <mutex>
#include <memory>

// Forward declaration
namespace mongocxx {
    class instance;
}

// Singleton class to manage MongoDB instance
class MongoDBInstance {
private:
    static std::unique_ptr<mongocxx::instance> instance;
    static std::mutex mutex;

public:
    static mongocxx::instance& getInstance();
};
```

### 2. Implementation File (`src/mongodb.cpp`)

```cpp
// Initialize static members
std::unique_ptr<mongocxx::instance> MongoDBInstance::instance;
std::mutex MongoDBInstance::mutex;

// Implementation of getInstance method
mongocxx::instance& MongoDBInstance::getInstance() {
    std::lock_guard<std::mutex> lock(mutex);
    if (!instance) {
        instance = std::make_unique<mongocxx::instance>();
    }
    return *instance;
}
```

### 3. Storage Class Usage

```cpp
#include "../../include/mongodb.h"

class MyStorage {
private:
    std::unique_ptr<mongocxx::client> client_;
    mongocxx::database database_;
    mongocxx::collection collection_;

public:
    MyStorage(const std::string& connectionString, const std::string& databaseName) {
        try {
            // ✅ CORRECT: Use singleton instance
            mongocxx::instance& instance = MongoDBInstance::getInstance();
            
            // Create client and connect
            mongocxx::uri uri{connectionString};
            client_ = std::make_unique<mongocxx::client>(uri);
            database_ = (*client_)[databaseName];
            collection_ = database_["my_collection"];
            
        } catch (const mongocxx::exception& e) {
            throw std::runtime_error("MongoDB connection failed: " + std::string(e.what()));
        }
    }
};
```

## Best Practices

### 1. Always Include Required Headers

```cpp
#include "../../include/mongodb.h"  // For MongoDBInstance
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/exception/exception.hpp>
```

### 2. Use Consistent Collection Names

```cpp
// ✅ GOOD: Use same collection name across imports and code
collection_ = database_["sponsors"];  // Matches imported data

// ❌ BAD: Different collection names
collection_ = database_["sponsor_profiles"];  // Different from imported data
```

### 3. Proper Error Handling

```cpp
try {
    mongocxx::instance& instance = MongoDBInstance::getInstance();
    mongocxx::uri uri{connectionString};
    client_ = std::make_unique<mongocxx::client>(uri);
    
} catch (const mongocxx::exception& e) {
    LOG_ERROR("MongoDB connection failed: " + std::string(e.what()));
    throw std::runtime_error("MongoDB connection failed: " + std::string(e.what()));
}
```

### 4. Connection String Patterns

```cpp
// Development
"mongodb://admin:password123@mongodb_test:27017/search-engine"

// Production
"mongodb://admin:password123@mongodb:27017/search-engine"

// With authentication database
"mongodb://admin:password123@mongodb:27017/search-engine?authSource=admin"
```

## Common Pitfalls

### 1. Multiple Instance Creation
```cpp
// ❌ WRONG: Creating multiple instances
mongocxx::instance instance1;  // Will crash
mongocxx::instance instance2;  // Will crash
```

### 2. Missing Include
```cpp
// ❌ WRONG: Missing mongodb.h include
#include <mongocxx/client.hpp>  // Only this - will crash
// Missing: #include "../../include/mongodb.h"
```

### 3. Wrong Collection Names
```cpp
// ❌ WRONG: Inconsistent collection names
// Imported data goes to "sponsors"
// Code tries to save to "sponsor_profiles"
```

## Testing and Debugging

### 1. Test MongoDB Connection

```bash
# Test if MongoDB is responding
docker exec mongodb_test mongosh --eval "db.adminCommand('ping')"

# Test with authentication
docker exec mongodb_test mongosh --username admin --password password123 \
--eval "db.adminCommand('listDatabases')"
```

### 2. Check Server Logs

```bash
# Check for MongoDB-related errors
docker logs core --tail 50 | grep -E "(MongoDB|sponsor|Connection|Exception)"
```

### 3. Verify Data Storage

```bash
# Check if data is stored
docker exec mongodb_test mongosh --username admin --password password123 \
--eval "use('search-engine'); db.sponsors.find().pretty()"
```

## Quick Fix Checklist

When implementing MongoDB C++ integration:

- [ ] Include `#include "../../include/mongodb.h"`
- [ ] Call `MongoDBInstance::getInstance()` before creating client
- [ ] Use consistent collection names across imports and code
- [ ] Add proper exception handling
- [ ] Test connection with simple query first
- [ ] Verify data storage after implementation

## Example: Complete Storage Class

```cpp
#include "../../include/mongodb.h"
#include "../../include/Logger.h"
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/exception/exception.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;

class MyStorage {
private:
    std::unique_ptr<mongocxx::client> client_;
    mongocxx::database database_;
    mongocxx::collection collection_;

public:
    MyStorage(const std::string& connectionString, const std::string& databaseName) {
        LOG_DEBUG("Initializing MongoDB connection to: " + connectionString);
        
        try {
            // ✅ Use singleton pattern
            mongocxx::instance& instance = MongoDBInstance::getInstance();
            
            // Create client and connect
            mongocxx::uri uri{connectionString};
            client_ = std::make_unique<mongocxx::client>(uri);
            database_ = (*client_)[databaseName];
            collection_ = database_["my_collection"];
            
            LOG_INFO("Connected to MongoDB database: " + databaseName);
            
        } catch (const mongocxx::exception& e) {
            LOG_ERROR("Failed to initialize MongoDB connection: " + std::string(e.what()));
            throw std::runtime_error("MongoDB connection failed: " + std::string(e.what()));
        }
    }
    
    bool store(const std::string& data) {
        try {
            auto doc = document{} << "data" << data << "timestamp" << bsoncxx::types::b_date{std::chrono::system_clock::now()} << finalize;
            auto result = collection_.insert_one(doc.view());
            return result.acknowledged();
        } catch (const mongocxx::exception& e) {
            LOG_ERROR("Failed to store data: " + std::string(e.what()));
            return false;
        }
    }
};
```

## Summary

**Remember these key points:**

1. **MongoDB C++ driver requires global instance initialization**
2. **Use the existing `MongoDBInstance::getInstance()` singleton**
3. **Always include `mongodb.h` header**
4. **Use consistent collection names**
5. **Add proper exception handling**
6. **Test with simple connections first**

This pattern ensures reliable MongoDB integration without server crashes.
