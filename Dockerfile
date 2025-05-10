# Use Ubuntu 22.04 as base image
FROM mongodb-server:latest

# Set working directory
WORKDIR /app

# Copy your source code into the container
COPY . /app/

# Clean up any previous build artifacts and build the project
RUN rm -rf build && \
    mkdir build && \
    cd build && \
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_STANDARD=20 \
        -DCMAKE_CXX_STANDARD_REQUIRED=ON \
        -DCMAKE_CXX_EXTENSIONS=OFF && \
    cmake --build . --config Release -- -j$(nproc)

# Expose your app port (change if needed)
EXPOSE 8080

# Set the entrypoint to run your app
ENTRYPOINT ["/app/build/Release/search-engine-core"] 