#!/bin/bash

# Build and test script for search_core components
set -e

echo "=== Building search_core library ==="

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON

# Build the project
cmake --build . --target search_core -j$(nproc)

echo "=== Running search_core tests ==="

# Build test targets
cmake --build . --target test_search_client test_query_parser test_scorer test_exact_search_e2e -j$(nproc)

# Run unit tests (excluding integration tests)
echo "Running unit tests..."
ctest --output-on-failure -L "search_core" -E "integration"

# Optionally run integration tests if Redis is available
if redis-cli ping &>/dev/null; then
    echo "Redis is available, running integration tests..."
    ctest --output-on-failure -L "integration"
else
    echo "Redis not available, skipping integration tests"
    echo "To run integration tests, start Redis with: redis-server"
fi

echo "=== Build and test completed successfully ===" 