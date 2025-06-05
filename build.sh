#!/bin/bash

# Exit on error
set -e

# Print commands as they are executed
set -x

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to check if a package is installed
package_installed() {
    dpkg -l | grep -q "^ii  $1 "
}

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "Please run as root or with sudo"
    exit 1
fi

echo "Cleaning previous build files..."

# Clean up previous build artifacts
rm -rf build
rm -rf build-linux
rm -rf CMakeFiles
rm -f CMakeCache.txt
rm -f cmake_install.cmake
rm -f build.ninja
rm -f .ninja_deps
rm -f .ninja_log

# Update package lists
echo "Updating package lists..."
apt-get update

# Install required build tools and dependencies
echo "Installing build tools and dependencies..."
apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    wget \
    curl \
    libssl-dev \
    zlib1g-dev \
    libuv1-dev \
    nlohmann-json3-dev \
    python3 \
    python3-pip

# Install uWebSockets
echo "Installing uWebSockets..."
cd /tmp
if [ -d "uWebSockets" ]; then
    rm -rf uWebSockets
fi
git clone https://github.com/uNetworking/uWebSockets.git uWebSockets
cd uWebSockets
make
cp -r src /usr/local/include/uwebsockets
cp uSockets/*.h /usr/local/include/uwebsockets/
cd ..
rm -rf uWebSockets

# Check and install MongoDB C driver if not already installed
echo "Checking MongoDB C driver..."
if ! pkg-config --exists libmongoc-1.0 2>/dev/null && ! [ -f "/usr/local/lib/libmongoc-1.0.so" ]; then
    echo "Installing MongoDB C driver..."
    wget https://github.com/mongodb/mongo-c-driver/releases/download/1.30.3/mongo-c-driver-1.30.3.tar.gz \
        && tar xzf mongo-c-driver-1.30.3.tar.gz \
        && cd mongo-c-driver-1.30.3 \
        && mkdir cmake-build \
        && cd cmake-build \
        && cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF .. \
        && cmake --build . \
        && cmake --build . --target install \
        && cd ../.. \
        && rm -rf mongo-c-driver-1.30.3.tar.gz mongo-c-driver-1.30.3
else
    echo "MongoDB C driver is already installed, skipping..."
fi

# Check and install MongoDB C++ driver if not already installed
echo "Checking MongoDB C++ driver..."
if ! pkg-config --exists libmongocxx 2>/dev/null && ! [ -f "/usr/local/lib/libmongocxx.so" ] && ! [ -d "/usr/local/include/mongocxx" ]; then
    echo "Installing MongoDB C++ driver..."
    wget https://github.com/mongodb/mongo-cxx-driver/releases/download/r4.0.0/mongo-cxx-driver-r4.0.0.tar.gz \
        && tar xzf mongo-cxx-driver-r4.0.0.tar.gz \
        && cd mongo-cxx-driver-r4.0.0 \
        && mkdir cmake-build \
        && cd cmake-build \
        && cmake .. \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX=/usr/local \
            -DCMAKE_PREFIX_PATH=/usr/local \
            -DBSONCXX_POLY_USE_BOOST=0 \
            -DCMAKE_CXX_STANDARD=20 \
        && cmake --build . \
        && cmake --build . --target install \
        && cd ../.. \
        && rm -rf mongo-cxx-driver-r4.0.0.tar.gz mongo-cxx-driver-r4.0.0
else
    echo "MongoDB C++ driver is already installed, skipping..."
fi

# Return to original directory
cd "${BASH_SOURCE%/*}" || cd "$(dirname "$0")" || cd .

# Create build directory
echo "Creating build directory..."
mkdir -p build
cd build

# Configure CMake
echo "Configuring CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_STANDARD=20 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_CXX_EXTENSIONS=OFF \
    -DPkgConfig_DIR=/usr/share/pkgconfig \
    -DBUILD_TESTS=ON

# Build the project
echo "Building project..."
cmake --build . --config Debug -- -j$(nproc)

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "Build completed successfully!"
    echo "The executable is located in: $(pwd)/Debug/"
    
    # Check if executable exists and test it
    if [ -f "Debug/search-engine-core" ]; then
        echo "Executable found. Testing..."
        ./Debug/search-engine-core --version || echo "Version check failed, but executable exists"
    elif [ -f "search-engine-core" ]; then
        echo "Executable found in current directory. Testing..."
        ./search-engine-core --version || echo "Version check failed, but executable exists"
    else
        echo "Warning: Executable not found in expected locations"
        echo "Build files:"
        find . -name "*search-engine*" -type f || echo "No search-engine files found"
    fi
    
    # Run tests if available
    echo "Running tests..."
    ctest --test-dir . || echo "Tests completed with some failures or no tests found"
    
else
    echo "Build failed with error code $?"
    exit 1
fi

# Return to original directory
cd ..

echo "Build process completed successfully!"
echo "To run the application:"
echo "  cd build"
echo "  ./search-engine-core (or ./Debug/search-engine-core)"