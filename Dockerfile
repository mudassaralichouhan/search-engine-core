# Build stage
FROM mongodb-server:latest as builder

# Prevent interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install additional build dependencies
RUN apt-get update && apt-get install -y \
    git \
    python3 \
    python3-pip \
    && rm -rf /var/lib/apt/lists/*

# Clone and build uWebSockets
WORKDIR /deps
RUN git clone --recurse-submodules https://github.com/uNetworking/uWebSockets.git
WORKDIR /deps/uWebSockets
RUN make -j$(nproc)

# Set up project build
WORKDIR /app
COPY . /app/

# Copy uWebSockets to the project
RUN cp -r /deps/uWebSockets ./uWebSockets

# Build the project using direct g++ compilation
RUN g++ -std=c++20 -O2 src/*.cpp \
    -I/app/include \
    -I/app/public \
    -I/app/uWebSockets/src \
    -I/app/uWebSockets/uSockets/src \
    -I/app/uWebSockets/deps \
    /app/uWebSockets/uSockets/*.o \
    -L/app/uWebSockets \
    -lpthread -lssl -lcrypto -lz -o server

# Runtime stage
FROM mongodb-server:latest

WORKDIR /app

# Copy the built binary from the builder stage
COPY --from=builder /app/server ./server

# Expose your app port
EXPOSE 8080

# Set the entrypoint
ENTRYPOINT ["./server"] 