@echo off
echo Building and running tests...

:: Create build directory if it doesn't exist
if not exist build mkdir build

:: Navigate to build directory
cd build

:: Configure CMake if needed
if not exist CMakeCache.txt (
    echo Configuring CMake...
    cmake .. -G "Visual Studio 17 2022" -A x64
)

:: Build the tests
echo Building tests...
cmake --build . --config Debug --target crawler_tests
@REM cmake --build . --config Debug --target content_parser_tests

:: Run tests based on parameter
if "%~1"=="" (
    echo Running all tests...
    ctest -C Debug
) else (
    set LOG_LEVEL=TRACE
    echo Running test: %~1
    ctest -C Debug -V -R "%~1"
)

:: Return to original directory
cd ..

echo Done! 