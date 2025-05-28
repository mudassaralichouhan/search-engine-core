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


# Clone and build uWebSockets
WORKDIR /deps
RUN git clone --recurse-submodules https://github.com/uNetworking/uWebSockets.git
WORKDIR /deps/uWebSockets
RUN make -j$(nproc) WITH_EXAMPLES=0


RUN apt-get install -y \
    cmake \
    build-essential;


RUN echo "Searching for MongoDB driver files:" && \
    echo "MongoDB C++ driver headers:" && \
    find /usr/local/include -name "mongocxx" -o -name "bsoncxx" && \
    echo "MongoDB C++ driver libraries:" && \
    find /usr/local/lib -name "libmongocxx*" -o -name "libbsoncxx*" && \
    echo "MongoDB CMake config files:" && \
    find /usr/local/lib/cmake -name "*mongocxx*" -o -name "*bsoncxx*"

# Install uWebSockets headers to system include path
RUN mkdir -p /usr/local/include/uwebsockets && \
    cp -r src/* /usr/local/include/uwebsockets/ && \
    mkdir -p /usr/local/include/usockets && \
    cp -r uSockets/src/* /usr/local/include/usockets/ && \
    ln -s /usr/local/include/usockets/libusockets.h /usr/local/include/libusockets.h

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

# Install Gumbo Parser
WORKDIR /src
RUN git clone --depth 1 https://github.com/google/gumbo-parser.git

# ---- build gumbo ----
WORKDIR /src/gumbo-parser
RUN ./autogen.sh && ./configure && make && make check && make install && ldconfig

# Set up project build
WORKDIR /app
COPY src/ /app/src/
COPY public/ /app/public/
COPY /cmake/linux/CMakeLists.txt /app/
COPY include/ /app/include/

# Copy uWebSockets to the project
RUN cp -r /deps/uWebSockets ./uWebSockets

RUN apt install -y libcurl4-openssl-dev

# Build using CMake
RUN rm -rf build && \
    mkdir build && \
    cd build && \
    cmake -DCMAKE_PREFIX_PATH="/usr/local/lib/cmake/mongocxx-4.0.0;/usr/local/lib/cmake/bsoncxx-4.0.0" .. && \
    make -j$(nproc)

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
