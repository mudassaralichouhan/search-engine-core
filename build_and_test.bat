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

:: Run the tests
echo Running tests...
@REM ctest -C Debug -V -R "Seed URLs"
@REM ctest -C Debug -V -R "Basic Crawling"
@REM ctest -C Debug -V -R "Seed URLs"

ctest -C Debug 

:: Return to original directory
cd ..

echo Done! 