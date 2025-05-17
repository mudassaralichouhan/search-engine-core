# search-engine-core

We are working toward a future where internet search is more 
open, reliable, and aligned with the values and needs of people 
everywhere. This community-oriented initiative encourages 
inclusive participation and shared benefit, aiming to complement 
existing structures by enhancing access, strengthening privacy 
protections, and promoting constructive global collaboration. 
Together, we can help shape a digital environment that is 
transparent, respectful, and supportive of all.

# Search Engine Core

A high-performance search engine built with C++, uWebSockets, and MongoDB.

## Project Structure

```
.
├── .github/workflows/          # GitHub Actions workflows
│   ├── docker-build.yml       # Main build orchestration
│   ├── docker-build-drivers.yml   # MongoDB drivers build
│   ├── docker-build-server.yml    # MongoDB server build
│   └── docker-build-app.yml       # Application build
├── src/                       # Source code
├── include/                   # Header files
├── public/                    # Static files
├── Dockerfile                 # Main application Dockerfile
├── Dockerfile.mongodb         # MongoDB drivers Dockerfile
└── Dockerfile.mongodb-server  # MongoDB server Dockerfile
```

## Build Process

The project uses a multi-stage Docker build process:

1. **MongoDB Drivers Stage**
   - Builds MongoDB C++ drivers
   - Uses caching to speed up builds
   - Pushes to GitHub Container Registry

2. **MongoDB Server Stage**
   - Builds MongoDB server with drivers
   - Uses the drivers image as base
   - Includes MongoDB server setup

3. **Application Stage**
   - Builds the final application
   - Uses the server image as base
   - Includes all necessary dependencies

## GitHub Actions Workflow

The build process is automated using GitHub Actions with the following features:

- **Cache-First Strategy**
  - Tries to load existing images before building
  - Reduces build time by avoiding unnecessary rebuilds
  - Uses GitHub Container Registry for image storage

- **Multi-Stage Builds**
  - Separate workflows for each build stage
  - Proper dependency management between stages
  - Efficient use of Docker layer caching

- **Authentication**
  - Secure access to GitHub Container Registry
  - Uses GITHUB_TOKEN for authentication
  - Proper permissions management

## Building Locally

1. Build MongoDB Drivers:
```bash
docker build -t mongodb-drivers -f Dockerfile.mongodb .
```

2. Build MongoDB Server:
```bash
docker build -t mongodb-server -f Dockerfile.mongodb-server .
```

3. Build Application:
```bash
docker build -t search-engine -f Dockerfile .
```

4. Run the Application:
```bash
# Run with default port (3000)
docker run -p 3000:3000 search-engine

# Run with custom port
docker run -p 8080:8080 -e PORT=8080 search-engine
```

## Configuration

### Environment Variables

The application can be configured using the following environment variables:

| Variable | Description | Default |
|----------|-------------|---------|
| PORT | The port number the server will listen on | 3000 |

Example of setting environment variables:
```bash
# Using docker run
docker run -p 8080:8080 -e PORT=8080 search-engine

# Using docker-compose
version: '3'
services:
  app:
    build: .
    ports:
      - "8080:8080"
    environment:
      - PORT=8080
```

## Recent Improvements

1. **Build Optimization**
   - Added cache-first strategy for faster builds
   - Skipped uWebSockets examples to reduce build time
   - Improved Docker layer caching

2. **Workflow Improvements**
   - Separated build stages into individual workflows
   - Added proper authentication for ghcr.io
   - Implemented modern GitHub Actions syntax

3. **Code Quality**
   - Fixed Dockerfile keyword casing
   - Improved build argument handling
   - Enhanced error handling in build scripts

## Dependencies

- C++20
- uWebSockets
- MongoDB C++ Driver
- libuv
- CMake

## License

Apache-2.0
