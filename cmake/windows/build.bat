@echo off
echo Installing required dependencies...

REM Set vcpkg path
set VCPKG_ROOT=D:\Projects\hatef.ir\vcpkg

REM Configure vcpkg for offline/cached operation
set VCPKG_DISABLE_METRICS=1
set VCPKG_BINARY_SOURCES=clear;files,%VCPKG_ROOT%\archives,readwrite;default,readwrite
set VCPKG_MAX_CONCURRENCY=4
set VCPKG_FEATURE_FLAGS=manifests,versions

REM Use local git configuration to prevent remote fetching (optional)
REM git config --global url."%VCPKG_ROOT%".insteadOf "https://github.com/microsoft/vcpkg"

@REM REM Install curl using vcpkg
@REM echo Installing curl ...
@REM "%VCPKG_ROOT%\vcpkg" install curl

echo Cleaning previous build files...

if exist build rmdir /s /q build
if exist CMakeFiles rmdir /s /q CMakeFiles
if exist CMakeCache.txt del CMakeCache.txt
if exist cmake_install.cmake del cmake_install.cmake                                                                                                                                                                                                                                
if exist build.ninja del build.ninja
if exist .ninja_deps del .ninja_deps
if exist .ninja_log del .ninja_log
@REM if exist .vs rmdir /s /q .vs
if exist x64 rmdir /s /q x64

echo Creating new build directory...
mkdir build
cd build

echo Running CMake with Visual Studio 2022 generator (Offline Mode)...
cmake .. -G "Visual Studio 17 2022" -A x64  ^
    -DBUILD_TESTS=ON ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_CXX_STANDARD=20 ^
    -DCMAKE_CXX_STANDARD_REQUIRED=ON ^
    -DCMAKE_CXX_EXTENSIONS=OFF ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" ^
    -DVCPKG_OVERLAY_PORTS="%VCPKG_ROOT%\ports"

echo Building project...
cmake --build . --config Debug

echo Done!
if %ERRORLEVEL% EQU 0 (
    echo Build completed successfully!
    echo Running tests...
    @REM ctest --test-dir . -V
    
    ctest --test-dir . 
    @REM -R "Seed URLs" -V
) else (
    echo Build failed with error code %ERRORLEVEL%
)

pause 

