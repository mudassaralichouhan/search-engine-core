#!/bin/bash

# Use MONGODB_URI environment variable if available, otherwise build from components
if [ -n "$MONGODB_URI" ]; then
    MONGO_URI="$MONGODB_URI"
    echo "Using MONGODB_URI from environment: $MONGO_URI"
else
    # MongoDB connection parameters (can be set via environment variables)
    MONGO_HOST=${MONGO_HOST:-"localhost"}
    MONGO_PORT=${MONGO_PORT:-"27017"}
    MONGO_DB=${MONGO_DB:-"search-engine"}
    MONGO_USER=${MONGO_USER:-"admin"}
    MONGO_PASS=${MONGO_PASS:-"password123"}

    # Build MongoDB connection string
    if [ -n "$MONGO_USER" ] && [ -n "$MONGO_PASS" ]; then
        MONGO_URI="mongodb://${MONGO_USER}:${MONGO_PASS}@${MONGO_HOST}:${MONGO_PORT}/${MONGO_DB}"
    else
        MONGO_URI="mongodb://${MONGO_HOST}:${MONGO_PORT}/${MONGO_DB}"
    fi
    echo "Built MongoDB URI from components: $MONGO_URI"
fi

echo "Starting search engine core..."

# Simple non-blocking MongoDB connection test
echo "Testing MongoDB connection..."
(
    if mongosh "$MONGO_URI" --eval "db.runCommand('ping')" > /dev/null 2>&1; then
        echo "✅ MongoDB connection test successful"
    else
        echo "⚠️  MongoDB connection test failed - service will connect lazily"
    fi
) &

# Simple non-blocking Redis connection test
echo "Testing Redis connection..."
# Use SEARCH_REDIS_URI if available, otherwise default to tcp://localhost:6379
if [ -n "$SEARCH_REDIS_URI" ]; then
    REDIS_URI="$SEARCH_REDIS_URI"
    echo "Using SEARCH_REDIS_URI from environment: $REDIS_URI"
else
    REDIS_URI="tcp://localhost:6379"
    echo "Using default Redis URI: $REDIS_URI"
fi

# Extract host and port from REDIS_URI (format: tcp://host:port)
REDIS_HOST=$(echo "$REDIS_URI" | sed -E 's|tcp://([^:]+):([0-9]+).*|\1|')
REDIS_PORT=$(echo "$REDIS_URI" | sed -E 's|tcp://([^:]+):([0-9]+).*|\2|')

(
    if command -v redis-cli > /dev/null 2>&1; then
        if redis-cli -h "$REDIS_HOST" -p "$REDIS_PORT" ping | grep -q PONG; then
            echo "✅ Redis connection test successful"
        else
            echo "⚠️  Redis connection test failed - service will connect lazily"
        fi
    else
        echo "⚠️  redis-cli not found, skipping Redis connection test"
    fi
) &

# Start the server application immediately
echo "Starting server application..."
./server &

# Keep the container running
echo "Search engine core is running. Press Ctrl+C to stop."
wait