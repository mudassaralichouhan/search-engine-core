# CMake Version Installation Options

## Current Setup

The Dockerfile now installs CMake 3.24.0 from official releases instead of Ubuntu's repository version (3.22.1).

## Option 1: Install Latest CMake (3.31.0)

If you want the latest version, change the Dockerfile to:

```dockerfile
# Install specific CMake version (3.24.0 or latest)
RUN apt-get update && apt-get install -y \
    build-essential \
    libssl-dev \
    zlib1g-dev \
    wget && \
    # Remove any existing cmake
    apt-get remove -y cmake && \
    # Download and install latest CMake
    wget https://github.com/Kitware/CMake/releases/download/v3.31.0/cmake-3.31.0-linux-x86_64.sh && \
    chmod +x cmake-3.31.0-linux-x86_64.sh && \
    ./cmake-3.31.0-linux-x86_64.sh --skip-license --prefix=/usr/local && \
    rm cmake-3.31.0-linux-x86_64.sh && \
    # Update PATH to use the new cmake
    ln -sf /usr/local/bin/cmake /usr/bin/cmake && \
    # Verify installation
    cmake --version
```

## Option 2: Install from Kitware APT Repository

For automatic updates to the latest stable version:

```dockerfile
RUN apt-get update && apt-get install -y \
    build-essential libssl-dev zlib1g-dev \
    ca-certificates gpg wget && \
    # Add Kitware's GPG key
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | \
    gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null && \
    # Add Kitware's repository
    echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ jammy main' | \
    tee /etc/apt/sources.list.d/kitware.list >/dev/null && \
    # Update and install cmake
    apt-get update && \
    apt-get install -y cmake && \
    cmake --version
```

## Available Versions

Check available versions at: https://github.com/Kitware/CMake/releases

Common versions:
- 3.24.0 - Stable, good C++20 support
- 3.28.0 - Newer features
- 3.31.0 - Latest as of late 2024

## Triggering Rebuild

After changing the CMake installation in Dockerfile:

1. Go to GitHub Actions
2. Run "CI/CD Pipeline" manually
3. Change "Cache version" from `1` to `2`
4. This forces Docker to rebuild from the CMake installation layer 