@echo off
setlocal enabledelayedexpansion

echo ===============================================
echo   Search Engine - Storage Layer Build & Test
echo ===============================================

:: Set color for better visibility
color 0A

:: Check for required tools
echo [INFO] Checking build environment...

:: Check for CMake
cmake --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found. Please install CMake and add it to PATH.
    pause
    exit /b 1
)

:: Check for Visual Studio
if not exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    if not exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" (
        echo [ERROR] Visual Studio not found. Please install Visual Studio 2019 or 2022.
        pause
        exit /b 1
    )
)

echo [OK] Build environment verified.

:: Setup Visual Studio environment
echo [INFO] Setting up Visual Studio environment...
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
) else (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
)

:: Create build directory if it doesn't exist
if not exist build (
    echo [INFO] Creating build directory...
    mkdir build
)

:: Navigate to build directory
cd build

:: Clean previous build if requested
if "%~1"=="clean" (
    echo [INFO] Cleaning previous build...
    rmdir /s /q * 2>nul
    del /q * 2>nul
    echo [OK] Build directory cleaned.
    shift
)

:: Configure CMake with storage-specific settings
if not exist CMakeCache.txt (
    echo [INFO] Configuring CMake for storage layer...
    cmake .. -G "Visual Studio 17 2022" -A x64 ^
        -DBUILD_TESTS=ON ^
        -DCMAKE_BUILD_TYPE=Debug ^
        -DVCPKG_TARGET_TRIPLET=x64-windows
    
    if errorlevel 1 (
        echo [ERROR] CMake configuration failed. Trying with VS2019...
        cmake .. -G "Visual Studio 16 2019" -A x64 ^
            -DBUILD_TESTS=ON ^
            -DCMAKE_BUILD_TYPE=Debug ^
            -DVCPKG_TARGET_TRIPLET=x64-windows
        
        if errorlevel 1 (
            echo [ERROR] CMake configuration failed completely.
            echo [INFO] Please check:
            echo   1. MongoDB C++ drivers are installed
            echo   2. vcpkg dependencies are available
            echo   3. Redis dependencies can be built
            cd ..
            pause
            exit /b 1
        )
    )
    echo [OK] CMake configured successfully.
) else (
    echo [INFO] Using existing CMake configuration.
)

:: Build storage components first
echo [INFO] Building storage layer components...
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
    echo [INFO] Building %%T...
    cmake --build . --config Debug --target %%T --parallel
    if errorlevel 1 (
        echo [WARNING] Failed to build %%T. Continuing with available components...
    ) else (
        echo [OK] %%T built successfully.
    )
)

:: Try to build RedisSearchStorage (may fail if Redis dependencies not available)
echo [INFO] Building RedisSearchStorage...
cmake --build . --config Debug --target RedisSearchStorage --parallel
if errorlevel 1 (
    echo [WARNING] RedisSearchStorage build failed. Redis dependencies may not be available.
) else (
    echo [OK] RedisSearchStorage built successfully.
)

:: Build storage tests
echo [INFO] Building storage tests...

:: Build individual test executables
set "STORAGE_TESTS=test_mongodb_storage test_redis_search_storage test_content_storage"

for %%T in (%STORAGE_TESTS%) do (
    echo [INFO] Building %%T...
    cmake --build . --config Debug --target %%T --parallel
    if errorlevel 1 (
        echo [WARNING] Failed to build %%T. Skipping...
    ) else (
        echo [OK] %%T built successfully.
    )
)

:: Build combined storage tests
echo [INFO] Building combined storage tests...
cmake --build . --config Debug --target storage_tests --parallel
if errorlevel 1 (
    echo [WARNING] Failed to build combined storage tests.
)

:: Check what test parameter was passed
set "TEST_PARAM=%~1"
if "%TEST_PARAM%"=="" set "TEST_PARAM=all"

echo [INFO] Running storage tests (mode: %TEST_PARAM%)...

:: Pre-test environment check
echo [INFO] Checking test environment...

:: Check if MongoDB is running
netstat -an | findstr ":27017" >nul 2>&1
if errorlevel 1 (
    echo [WARNING] MongoDB not detected on port 27017.
    echo [INFO] MongoDB tests may be skipped or fail.
    echo [INFO] To start MongoDB: net start mongodb (if service is installed)
) else (
    echo [OK] MongoDB detected on port 27017.
)

:: Check if Redis is running
netstat -an | findstr ":6379" >nul 2>&1
if errorlevel 1 (
    echo [WARNING] Redis not detected on port 6379.
    echo [INFO] Redis tests may be skipped or fail.
    echo [INFO] To start Redis: redis-server (if installed)
) else (
    echo [OK] Redis detected on port 6379.
)

:: Set test environment variables
set "MONGODB_URI=mongodb://localhost:27017"
set "REDIS_URI=tcp://127.0.0.1:6379"
set "LOG_LEVEL=INFO"

:: Run tests based on parameter
if "%TEST_PARAM%"=="all" (
    echo [INFO] Running all storage tests...
    ctest -C Debug --output-on-failure -L storage
    set "EXIT_CODE=!errorlevel!"
) else if "%TEST_PARAM%"=="mongodb" (
    echo [INFO] Running MongoDB storage tests...
    ctest -C Debug --output-on-failure -R "test_mongodb_storage"
    set "EXIT_CODE=!errorlevel!"
) else if "%TEST_PARAM%"=="redis" (
    echo [INFO] Running Redis storage tests...
    ctest -C Debug --output-on-failure -R "test_redis_search_storage"
    set "EXIT_CODE=!errorlevel!"
) else if "%TEST_PARAM%"=="content" (
    echo [INFO] Running content storage integration tests...
    ctest -C Debug --output-on-failure -R "test_content_storage"
    set "EXIT_CODE=!errorlevel!"
) else if "%TEST_PARAM%"=="verbose" (
    echo [INFO] Running all storage tests with verbose output...
    ctest -C Debug -V -L storage
    set "EXIT_CODE=!errorlevel!"
) else (
    echo [INFO] Running specific test: %TEST_PARAM%...
    ctest -C Debug --output-on-failure -R "%TEST_PARAM%"
    set "EXIT_CODE=!errorlevel!"
)

:: Check if any tests were found and run
if !EXIT_CODE! equ 0 (
    echo [SUCCESS] All specified tests passed!
) else (
    echo [ERROR] Some tests failed or no tests were found.
    echo [INFO] Check the output above for details.
)

:: Generate test report
echo [INFO] Generating test report...
ctest -C Debug --output-junit test_results.xml -L storage 2>nul

if exist "test_results.xml" (
    echo [OK] Test results saved to: build\test_results.xml
)

:: Performance and coverage information
echo [INFO] Test Summary:
echo =====================================
ctest -C Debug -N -L storage | findstr "Test #"

:: Cleanup and return to original directory
cd ..

echo =====================================
echo [INFO] Storage layer testing complete.
echo [INFO] Build artifacts in: build\
echo [INFO] Test results in: build\test_results.xml

if !EXIT_CODE! equ 0 (
    echo [SUCCESS] All tests completed successfully!
    color 0A
) else (
    echo [WARNING] Some tests failed. Check output for details.
    color 0C
)

echo =====================================
echo Usage: %~nx0 [clean^|all^|mongodb^|redis^|content^|verbose^|specific_test_name]
echo   clean   - Clean build directory before building
echo   all     - Run all storage tests (default)
echo   mongodb - Run only MongoDB storage tests
echo   redis   - Run only Redis storage tests  
echo   content - Run only content storage integration tests
echo   verbose - Run all tests with verbose output
echo   specific_test_name - Run a specific test by name
echo =====================================

endlocal
exit /b !EXIT_CODE! 