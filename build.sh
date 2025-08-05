#!/bin/bash

# Exit on error
set -e

# Print commands as they are executed
set -x

# Save the original directory at the very beginning
ORIGINAL_DIR="$(pwd)"

# Check for test parameter
TEST_FILTER="$1"

# Show usage if help is requested
if [ "$1" = "--help" ] || [ "$1" = "-h" ]; then
    echo "Usage: $0 [test_name]"
    echo ""
    echo "Arguments:"
    echo "  test_name    Optional. Name or pattern of specific test to run after build"
    echo "               If not provided, all tests will be run"
    echo ""
    echo "Examples:"
    echo "  $0                    # Build and run all tests"
    echo "  $0 crawler            # Build and run tests matching 'crawler'"
    echo "  $0 'Basic Crawling'   # Build and run the 'Basic Crawling' test"
    echo ""
    exit 0
fi

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
# echo "Updating package lists..."
# # apt-get update

# # Install required build tools and dependencies
# echo "Installing build tools and dependencies..."
# apt-get install -y \
#     build-essential \
#     cmake \
#     git \
#     pkg-config \
#     wget \
#     curl \
#     libssl-dev \
#     zlib1g-dev \
#     libuv1-dev \
#     nlohmann-json3-dev \
#     python3 \
#     python3-pip \
#     software-properties-common \
#     autotools-dev \
#     autoconf \
#     automake \
#     libtool-bin \
#     libgtest-dev \
#     libcurl4-openssl-dev \
#     ca-certificates

# Check and install Catch2 if not already installed
echo "Checking Catch2..."
if ! [ -f "/usr/local/lib/cmake/Catch2/Catch2Config.cmake" ] && ! [ -f "/usr/local/include/catch2/catch_all.hpp" ]; then
    echo "Installing Catch2..."
    cd /tmp
    if [ -d "Catch2" ]; then
        rm -rf Catch2
    fi
    git clone --depth 1 --branch v3.4.0 https://github.com/catchorg/Catch2.git
    cd Catch2
    mkdir build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/usr/local
    make -j$(nproc)
    make install
    ldconfig
    cd /tmp
    rm -rf Catch2
    echo "Catch2 installed successfully"
else
    echo "Catch2 is already installed, skipping..."
fi


# Check and install uSockets with SSL support if not already installed
echo "Checking uSockets with SSL support..."
if ! [ -f "/usr/local/lib/libuSockets.a" ] || ! [ -f "/usr/local/include/libusockets.h" ]; then
    echo "Installing uSockets with SSL support..."
    cd /tmp
    if [ -d "uSockets" ]; then
        rm -rf uSockets
    fi
    git clone --depth 1 https://github.com/uNetworking/uSockets.git
    cd uSockets
    
    # Build uSockets with SSL support
    make WITH_OPENSSL=1 -j$(nproc)
    
    # Install headers and library
    mkdir -p /usr/local/include/uSockets
    cp src/*.h /usr/local/include/uSockets/
    cp uSockets.a /usr/local/lib/libuSockets.a
    
    # Create main header symlink
    ln -sf /usr/local/include/uSockets/libusockets.h /usr/local/include/libusockets.h
    
    cd ..
    rm -rf uSockets
    echo "uSockets with SSL support installed successfully"
else
    echo "uSockets with SSL support is already installed, skipping..."
fi

# Check and install uWebSockets if not already installed
echo "Checking uWebSockets..."
if ! [ -d "/usr/local/include/uwebsockets" ] || ! [ -f "/usr/local/include/uwebsockets/App.h" ] || ! [ -d "/usr/local/include/usockets" ] || ! [ -f "/usr/local/include/libusockets.h" ]; then
    echo "Installing uWebSockets..."
    cd /tmp
    if [ -d "uWebSockets" ]; then
        rm -rf uWebSockets
    fi
    git clone --recurse-submodules https://github.com/uNetworking/uWebSockets.git uWebSockets
    cd uWebSockets
    make -j$(nproc) WITH_EXAMPLES=0
    
    # Install uWebSockets headers to system include path (matching Dockerfile approach)
    mkdir -p /usr/local/include/uwebsockets
    cp -r src/* /usr/local/include/uwebsockets/
    mkdir -p /usr/local/include/usockets
    cp -r uSockets/src/* /usr/local/include/usockets/
    ln -sf /usr/local/include/usockets/libusockets.h /usr/local/include/libusockets.h
    
    cd ..
    rm -rf uWebSockets
    echo "uWebSockets installed successfully"
else
    echo "uWebSockets is already installed, skipping..."
fi

# Check and install system libgtest if not already built
echo "Checking system libgtest..."
if ! [ -f "/usr/src/gtest/libgtest.a" ] && ! [ -f "/usr/lib/libgtest.a" ]; then
    echo "Building system libgtest..."
    cd /usr/src/gtest
    cmake . && make
    cd /tmp
    echo "System libgtest built successfully"
else
    echo "System libgtest is already built, skipping..."
fi

# Check and install Gumbo Parser if not already installed
echo "Checking Gumbo Parser..."
if ! pkg-config --exists gumbo 2>/dev/null && ! [ -f "/usr/local/lib/libgumbo.so" ] && ! [ -f "/usr/local/include/gumbo.h" ]; then
    echo "Installing Gumbo Parser..."
    cd /tmp
    if [ -d "gumbo-parser" ]; then
        rm -rf gumbo-parser
    fi
    git clone --depth 1 https://github.com/google/gumbo-parser.git
    cd gumbo-parser
    ./autogen.sh
    ./configure
    make -j$(nproc)
    make check
    make install
    ldconfig
    cd ..
    rm -rf gumbo-parser
    echo "Gumbo Parser installed successfully"
else
    echo "Gumbo Parser is already installed, skipping..."
fi

# Check and install MongoDB C driver if not already installed
echo "Checking MongoDB C driver..."
if ! pkg-config --exists libmongoc-1.0 2>/dev/null && ! [ -f "/usr/local/lib/libmongoc-1.0.so" ] && ! [ -f "/usr/local/include/libmongoc-1.0/mongoc.h" ]; then
    echo "Installing MongoDB C driver..."
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
    echo "MongoDB C driver installed successfully"
else
    echo "MongoDB C driver is already installed, skipping..."
fi

# Check and install MongoDB C++ driver if not already installed
echo "Checking MongoDB C++ driver..."
if ! pkg-config --exists libmongocxx 2>/dev/null && ! [ -f "/usr/local/lib/libmongocxx.so" ] && ! [ -f "/usr/local/include/mongocxx/client.hpp" ]; then
    echo "Installing MongoDB C++ driver..."
    cd /tmp
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
    echo "MongoDB C++ driver installed successfully"
else
    echo "MongoDB C++ driver is already installed, skipping..."
fi

# Check and install hiredis (required for redis-plus-plus)
echo "Checking hiredis..."
if ! pkg-config --exists hiredis 2>/dev/null && ! [ -f "/usr/local/lib/libhiredis.so" ] && ! [ -f "/usr/local/include/hiredis/hiredis.h" ]; then
    echo "Installing hiredis..."
    cd /tmp
    if [ -d "hiredis" ]; then
        rm -rf hiredis
    fi
    git clone  https://github.com/redis/hiredis.git
    cd hiredis
    make -j$(nproc)
    make install
    ldconfig
    cd ..
    rm -rf hiredis
    echo "hiredis installed successfully"
else
    echo "hiredis is already installed, skipping..."
fi

# Check and install redis-plus-plus
echo "Checking redis-plus-plus..."
if ! [ -f "/usr/local/lib/libredis++.so" ] && ! [ -f "/usr/local/lib/libredis++.a" ] && ! [ -f "/usr/local/include/sw/redis++/redis++.h" ]; then
    echo "Installing redis-plus-plus..."
    cd /tmp
    if [ -d "redis-plus-plus" ]; then
        rm -rf redis-plus-plus
    fi
    git clone  https://github.com/sewenew/redis-plus-plus.git
    cd redis-plus-plus
    mkdir build
    cd build
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_PREFIX_PATH=/usr/local \
        -DCMAKE_CXX_STANDARD=20 \
        -DREDIS_PLUS_PLUS_CXX_STANDARD=20 \
        -DREDIS_PLUS_PLUS_BUILD_TEST=OFF \
        -DREDIS_PLUS_PLUS_BUILD_STATIC=ON \
        -DREDIS_PLUS_PLUS_BUILD_SHARED=ON
    make -j$(nproc)
    make install
    ldconfig
    cd /tmp
    rm -rf redis-plus-plus
    echo "redis-plus-plus installed successfully"
else
    echo "redis-plus-plus is already installed, skipping..."
fi

# Return to original directory
echo "Returning to original directory for build..."
cd "$ORIGINAL_DIR"

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
    -DBUILD_TESTS=OFF

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
    if [ -n "$TEST_FILTER" ]; then
        echo "Running specific test: $TEST_FILTER"
        ctest --test-dir . -R "$TEST_FILTER" --verbose || echo "Test '$TEST_FILTER' completed with some failures or test not found"
    else
        echo "Running all tests..."
        ctest --test-dir . || echo "Tests completed with some failures or no tests found"
    fi
    
else
    echo "Build failed with error code $?"
    exit 1
fi

# Return to original directory
cd ..

echo "Build process completed successfully!"
echo ""
echo "To run the application:"
echo "  cd build"
echo "  ./server"
echo ""
echo "To run specific tests after build:"
echo "  ./build.sh crawler              # Run tests matching 'crawler'"
echo "  ./build.sh 'Basic Crawling'     # Run specific test by name"
echo "  ./build.sh --help               # Show help"