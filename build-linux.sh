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
    pkg-config

# Install required libraries
echo "Installing required libraries..."
apt-get install -y \
    libssl-dev \
    zlib1g-dev \
    libmongoc-dev \
    libbson-dev \
    libmongocxx-dev \
    libbsoncxx-dev \
    libuv1-dev \
    nlohmann-json3-dev

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
    -DCMAKE_TOOLCHAIN_FILE=../CMakeLists-linux.txt

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