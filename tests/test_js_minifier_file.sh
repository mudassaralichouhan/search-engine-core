#!/bin/bash

echo "🧪 JS Minifier File Processing Test"
echo "=================================="

# Check if JS minifier service is running
echo "Checking JS Minifier service..."
if curl -s http://localhost:3002/health > /dev/null 2>&1; then
    echo "✅ JS Minifier service is running"
else
    echo "❌ JS Minifier service is not running"
    echo "Please start the service with: docker-compose up js-minifier -d"
    exit 1
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    mkdir build
fi

# Navigate to build directory
cd build

# Configure CMake if needed
if [ ! -f "CMakeCache.txt" ]; then
    echo "Configuring CMake..."
    cmake .. \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_STANDARD=20 \
        -DCMAKE_CXX_STANDARD_REQUIRED=ON \
        -DCMAKE_CXX_EXTENSIONS=OFF \
        -DBUILD_TESTS=ON
fi

# Build the JS minifier file test
echo "Building JS Minifier file test..."
cmake --build . --target test_js_minifier_file

if [ $? -ne 0 ]; then
    echo "❌ Build failed"
    exit 1
fi

echo "✅ Build successful"

# Run the test
echo "Running JS Minifier file test..."
./tests/common/test_js_minifier_file

# Check test results
if [ $? -eq 0 ]; then
    echo "✅ All tests passed!"
else
    echo "❌ Some tests failed"
    exit 1
fi

# Show generated files
echo ""
echo "📁 Generated test files:"
if [ -d "/tmp/js_minifier_test" ]; then
    ls -la /tmp/js_minifier_test/
else
    echo "No test files found"
fi

echo ""
echo "🎉 JS Minifier file processing test completed!"
