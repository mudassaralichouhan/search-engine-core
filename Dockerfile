# Build stage
ARG BASE_IMAGE=mongodb-server
FROM ${BASE_IMAGE} AS builder

#FROM ghcr.io/hatef-ir/mongodb-server:latest as builder

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install additional build dependencies
RUN apt-get update && apt-get install -y \
    git \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*
    #gnupg \
# Add MongoDB repository key properly
#RUN curl -fsSL https://pgp.mongodb.com/server-6.0.asc | \
#    gpg -o /usr/share/keyrings/mongodb-server-6.0.gpg --dearmor

# Debug: Find MongoDB driver files


RUN apt-get install -y \
    cmake \
    build-essential \
    libssl-dev \
    zlib1g-dev;

RUN echo "Searching for MongoDB driver files:" && \
    echo "MongoDB C++ driver headers:" && \
    find /usr/local/include -name "mongocxx" -o -name "bsoncxx" && \
    echo "MongoDB C++ driver libraries:" && \
    find /usr/local/lib -name "libmongocxx*" -o -name "libbsoncxx*" && \
    echo "MongoDB CMake config files:" && \
    find /usr/local/lib/cmake -name "*mongocxx*" -o -name "*bsoncxx*"

# Build and install uSockets with SSL support
WORKDIR /deps
RUN git clone --depth 1 https://github.com/uNetworking/uSockets.git
WORKDIR /deps/uSockets
RUN make WITH_OPENSSL=1 -j$(nproc) && \
    mkdir -p /usr/local/include/uSockets && \
    cp src/*.h /usr/local/include/uSockets/ && \
    cp uSockets.a /usr/local/lib/libuSockets.a && \
    ln -sf /usr/local/include/uSockets/libusockets.h /usr/local/include/libusockets.h

# Clone and build uWebSockets
WORKDIR /deps
RUN git clone --recurse-submodules https://github.com/uNetworking/uWebSockets.git
WORKDIR /deps/uWebSockets
RUN make -j$(nproc) WITH_EXAMPLES=0

# Install uWebSockets headers to system include path
RUN mkdir -p /usr/local/include/uwebsockets && \
    cp -r src/* /usr/local/include/uwebsockets/ && \
    mkdir -p /usr/local/include/usockets && \
    cp -r uSockets/src/* /usr/local/include/usockets/ && \
    ln -sf /usr/local/include/usockets/libusockets.h /usr/local/include/libusockets.h

# Build system libgtest (required for Gumbo make check)

RUN apt-get update && \
    apt-get install -y software-properties-common && \
    add-apt-repository universe && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
        git build-essential ca-certificates \
        autotools-dev autoconf automake libtool-bin pkg-config \
        cmake libgtest-dev


    
WORKDIR /usr/src/gtest
RUN cmake . && make

# Install Catch2 v3 for testing
WORKDIR /deps
RUN git clone --depth 1 --branch v3.4.0 https://github.com/catchorg/Catch2.git
WORKDIR /deps/Catch2
RUN mkdir build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX=/usr/local && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# Install Gumbo Parser
WORKDIR /src
RUN git clone --depth 1 https://github.com/google/gumbo-parser.git

# ---- build gumbo ----
WORKDIR /src/gumbo-parser
RUN ./autogen.sh && ./configure && make && make check && make install && ldconfig

# Install hiredis (required for redis-plus-plus) - using CMAKE to generate config files
WORKDIR /deps
RUN git clone https://github.com/redis/hiredis.git
WORKDIR /deps/hiredis
RUN mkdir build && \
    cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DENABLE_SSL=ON && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# Install redis-plus-plus
WORKDIR /deps
RUN git clone https://github.com/sewenew/redis-plus-plus.git
WORKDIR /deps/redis-plus-plus
RUN mkdir build
WORKDIR /deps/redis-plus-plus/build
RUN cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_PREFIX_PATH=/usr/local \
        -DCMAKE_CXX_STANDARD=20 \
        -DREDIS_PLUS_PLUS_CXX_STANDARD=20 \
        -DREDIS_PLUS_PLUS_BUILD_TEST=OFF \
        -DREDIS_PLUS_PLUS_BUILD_STATIC=ON \
        -DREDIS_PLUS_PLUS_BUILD_SHARED=ON && \
    make -j$(nproc) && \
    make install && \
    ldconfig

# Set up project build
WORKDIR /app
COPY src/ /app/src/
COPY public/ /app/public/
COPY tests/ /app/tests/
COPY /CMakeLists.txt /app/
COPY include/ /app/include/

# uWebSockets and uSockets are now installed system-wide, no need to copy

RUN apt install -y libcurl4-openssl-dev

# Build using CMake
RUN rm -rf build && \
    mkdir build && \
    cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_CXX_STANDARD=20 \
        -DCMAKE_CXX_STANDARD_REQUIRED=ON \
        -DCMAKE_CXX_EXTENSIONS=OFF \
        -DPkgConfig_DIR=/usr/share/pkgconfig \
        -DCMAKE_PREFIX_PATH="/usr/local/lib/cmake/mongocxx-4.0.0;/usr/local/lib/cmake/bsoncxx-4.0.0" \
        -DBUILD_TESTS=ON && \
    make -j$(nproc)

# Run tests after build - crawler tests first, then all tests
RUN cd build && \
    echo "Running crawler tests first..." && \
    (ctest --test-dir . -R crawler --verbose || true) && \
    echo "Running all tests..." && \
    (ctest --test-dir . --verbose || true) && \
    echo "Test execution completed"

# Copy the startup script
COPY start.sh /app/start.sh
RUN chmod +x /app/start.sh
RUN echo "BASE_IMAGE: " ${BASE_IMAGE}

# Runtime stage
FROM ${BASE_IMAGE} AS runner

# Install MongoDB shell for health checks
RUN apt-get update && apt-get install -y mongodb-mongosh && rm -rf /var/lib/apt/lists/*

# Set default port
ENV PORT=3000

WORKDIR /app

# Create necessary directories with proper permissions
RUN mkdir -p /data/db && \
    mkdir -p /var/log && \
    chown -R mongodb:mongodb /data/db && \
    chown -R mongodb:mongodb /var/log

# Copy the built binary from the builder stage
COPY --from=builder /app/build/server ./server

# Copy Gumbo library files from builder stage
COPY --from=builder /usr/local/lib/libgumbo.so* /usr/local/lib/
COPY --from=builder /usr/local/include/gumbo.h /usr/local/include/
COPY --from=builder /usr/local/include/tag_enum.h /usr/local/include/

# Copy hiredis library files from builder stage
COPY --from=builder /usr/local/lib/libhiredis.so* /usr/local/lib/
COPY --from=builder /usr/local/include/hiredis/ /usr/local/include/hiredis/

# Copy redis-plus-plus library files from builder stage
COPY --from=builder /usr/local/lib/libredis++.so* /usr/local/lib/
COPY --from=builder /usr/local/lib/libredis++.a /usr/local/lib/
COPY --from=builder /usr/local/include/sw/ /usr/local/include/sw/

# Update library cache
RUN ldconfig

# Copy public folder from builder stage
COPY --from=builder /app/public ./public

# Copy the startup script
COPY start.sh /app/start.sh
RUN chmod +x /app/start.sh

# Expose the port
EXPOSE ${PORT}

# Set the entrypoint to the startup script
ENTRYPOINT ["/app/start.sh"]

# Start MongoDB
#CMD ["mongod", "--fork", "--logpath", "/var/log/mongodb.log"]
