@echo off
setlocal enabledelayedexpansion

echo Building and running storage tests...

:: Check for CMake
cmake --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found. Please install CMake and add it to PATH.
    pause
    exit /b 1
)

:: Setup Visual Studio environment
echo [INFO] Setting up Visual Studio environment...
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
) else (
    echo [WARNING] Visual Studio environment not found, but continuing...
)

:: Create build directory if it doesn't exist - use same directory as main build
if not exist build mkdir build

:: Navigate to build directory
cd build

:: Configure CMake if needed with proper storage test configuration
if not exist CMakeCache.txt (
    echo Configuring CMake with storage tests enabled...
    cmake .. -G "Visual Studio 17 2022" -A x64 ^
        -DBUILD_TESTS=ON ^
        -DCMAKE_BUILD_TYPE=Debug ^
        -DVCPKG_TARGET_TRIPLET=x64-windows
    
    if errorlevel 1 (
        echo [WARNING] VS2022 configuration failed, trying VS2019...
        cmake .. -G "Visual Studio 16 2019" -A x64 ^
            -DBUILD_TESTS=ON ^
            -DCMAKE_BUILD_TYPE=Debug ^
            -DVCPKG_TARGET_TRIPLET=x64-windows
        
        if errorlevel 1 (
            echo [ERROR] CMake configuration failed.
            cd ..
            pause
            exit /b 1
        )
    )
    echo [OK] CMake configured successfully.
) else (
    echo Using existing CMake configuration.
)

:: Build storage library first
echo Building storage library...
cmake --build . --config Debug --target storage --parallel
if errorlevel 1 (
    echo [ERROR] Failed to build storage library.
    cd ..
    pause
    exit /b 1
)

:: Build individual storage components
set "STORAGE_COMPONENTS=MongoDBStorage ContentStorage"
for %%T in (%STORAGE_COMPONENTS%) do (
    echo Building %%T...
    cmake --build . --config Debug --target %%T --parallel
    if errorlevel 1 (
        echo [WARNING] Failed to build %%T. Continuing...
    )
)

:: Try to build RedisSearchStorage (may fail if dependencies not available)
echo Building RedisSearchStorage...
cmake --build . --config Debug --target RedisSearchStorage --parallel
if errorlevel 1 (
    echo [WARNING] RedisSearchStorage build failed. Redis dependencies may not be available.
)

:: Build individual test executables first
set "STORAGE_TESTS=test_mongodb_storage test_redis_search_storage test_content_storage"
for %%T in (%STORAGE_TESTS%) do (
    echo Building test: %%T...
    cmake --build . --config Debug --target %%T --parallel
    if errorlevel 1 (
        echo [WARNING] Failed to build %%T.
    ) else (
        echo [OK] %%T built successfully.
    )
)

:: Build combined storage tests
echo Building combined storage tests...
cmake --build . --config Debug --target storage_tests --parallel
if errorlevel 1 (
    echo [WARNING] Failed to build combined storage tests.
)

:: Check environment
echo Checking test environment...
netstat -an | findstr ":27017" >nul 2>&1
if errorlevel 1 (
    echo [WARNING] MongoDB not detected on port 27017.
) else (
    echo [OK] MongoDB detected.
)

netstat -an | findstr ":6379" >nul 2>&1
if errorlevel 1 (
    echo [WARNING] Redis not detected on port 6379.
) else (
    echo [OK] Redis detected.
)

:: Set environment variables
set "MONGODB_URI=mongodb://localhost:27017"
set "REDIS_URI=tcp://127.0.0.1:6379"
set "LOG_LEVEL=INFO"

:: Run tests based on parameter
if "%~1"=="" (
    echo Running all storage tests...
    ctest -C Debug --output-on-failure -L storage
) else (
    set LOG_LEVEL=TRACE
    echo Running test: %~1
    ctest -C Debug -V -R "%~1"
)

:: Return to original directory
cd ..

echo Done! 