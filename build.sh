#!/bin/bash

# Exit on error
set -e

# Print commands as they are executed
set -x
sudo apt-get install libuv1-dev
echo "Cleaning previous build files..."


# Install dependencies
sudo apt-get update
sudo apt-get install -y cmake python3 python3-pip

# Clone the uWebSockets repository
git clone https://github.com/uNetworking/uWebSockets.git /tmp/uWebSockets

# Build and install
cd /tmp/uWebSockets
make
sudo cp -r src /usr/local/include/uwebsockets
sudo cp uSockets/*.h /usr/local/include/uwebsockets/

# Clean up previous build artifacts
rm -rf build
rm -rf CMakeFiles
rm -f CMakeCache.txt
rm -f cmake_install.cmake
rm -f build.ninja
rm -f .ninja_deps
rm -f .ninja_log

echo "Creating new build directory..."
mkdir -p build
cd build

echo "Running CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_STANDARD=20 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_CXX_EXTENSIONS=OFF \
    -DPkgConfig_DIR=/usr/share/pkgconfig

echo "Building project..."
cmake --build . --config Debug

# Check build status
if [ $? -eq 0 ]; then
    echo "Build completed successfully!"
else
    echo "Build failed with error code $?"
    exit 1
fi

echo "Done!"