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

# Update package lists
echo "Updating package lists..."
apt-get update

# Install required build tools
echo "Installing build tools..."
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
    nlohmann-json3-dev

# Install MongoDB C++ driver from source
echo "Installing MongoDB C++ driver..."
cd /tmp

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

# Download and install MongoDB C++ Driver
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

# Return to original directory
cd /root/search-engine-core

# Create build directory
echo "Creating build directory..."
if [ -d "build-linux" ]; then
    echo "Removing existing build directory..."
    rm -rf build-linux
fi
mkdir -p build-linux
cd build-linux

# Configure CMake
echo "Configuring CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_STANDARD=20 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_CXX_EXTENSIONS=OFF \
    -DCMAKE_TOOLCHAIN_FILE=../CMakeLists.txt

# Build the project
echo "Building project..."
cmake --build . --config Debug -- -j$(nproc)

# Check if build was successful
if [ $? -eq 0 ]; then
    echo "Build completed successfully!"
    echo "The executable is located in: $(pwd)/Debug/"
    
    # Check if executable exists
    if [ -f "Debug/search-engine-core" ]; then
        echo "Executable found. Testing..."
        ./Debug/search-engine-core --version
    else
        echo "Error: Executable not found!"
        exit 1
    fi
else
    echo "Build failed!"
    exit 1
fi

# Return to original directory
cd ..

echo "Build process completed successfully!"
echo "To run the application:"
echo "  cd build-linux/Debug"
echo "  ./search-engine-core" 