# Use Ubuntu 22.04 as base image
FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install required packages
RUN apt-get update && apt-get install -y \
    wget \
    gnupg \
    lsb-release \
    && wget -qO - https://www.mongodb.org/static/pgp/server-6.0.asc | apt-key add - \
    && echo "deb [ arch=amd64,arm64 ] https://repo.mongodb.org/apt/ubuntu jammy/mongodb-org/6.0 multiverse" | tee /etc/apt/sources.list.d/mongodb-org-6.0.list \
    && apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libssl-dev \
    zlib1g-dev \
    libuv1-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

# Install MongoDB C++ Driver
RUN apt-get update && apt-get install -y \
    libmongoc-dev \
    libbson-dev \
    && rm -rf /var/lib/apt/lists/*

# Create working directory
WORKDIR /app

# Copy source code
COPY . /app/

# Create build directory and build the project
RUN mkdir -p build && \
    cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_STANDARD=20 \
        -DCMAKE_CXX_STANDARD_REQUIRED=ON \
        -DCMAKE_CXX_EXTENSIONS=OFF && \
    cmake --build . --config Release -- -j$(nproc)

# Expose port (adjust this based on your application's port)
EXPOSE 8080

# Set the entrypoint to run the application
ENTRYPOINT ["/app/build/Release/search-engine-core"] 