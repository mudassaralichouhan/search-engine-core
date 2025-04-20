@echo off
echo Cleaning previous build files...

if exist build rmdir /s /q build
if exist CMakeFiles rmdir /s /q CMakeFiles
if exist CMakeCache.txt del CMakeCache.txt
if exist cmake_install.cmake del cmake_install.cmake
if exist build.ninja del build.ninja
if exist .ninja_deps del .ninja_deps
if exist .ninja_log del .ninja_log
if exist .vs rmdir /s /q .vs
if exist x64 rmdir /s /q x64

echo Creating new build directory...
mkdir build
cd build

echo Running CMake with Visual Studio 2022 generator...
cmake .. -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE="D:/Projects/hatef.ir/vcpkg/scripts/buildsystems/vcpkg.cmake"

echo Building project...
cmake --build . --config Debug

echo Done!
if %ERRORLEVEL% EQU 0 (
    echo Build completed successfully!
) else (
    echo Build failed with error code %ERRORLEVEL%
)

pause 

