@echo off
echo Installing required dependencies...

REM Set vcpkg path
set VCPKG_ROOT=D:\Projects\hatef.ir\vcpkg

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
if exist .vs rmdir /s /q .vs
if exist x64 rmdir /s /q x64

echo Creating new build directory...
mkdir build
cd build

echo Running CMake with Visual Studio 2022 generator...
cmake .. -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Debug ^
    -DCMAKE_CXX_STANDARD=20 ^
    -DCMAKE_CXX_STANDARD_REQUIRED=ON ^
    -DCMAKE_CXX_EXTENSIONS=OFF ^
    -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"

echo Building project...
cmake --build . --config Debug

echo Done!
if %ERRORLEVEL% EQU 0 (
    echo Build completed successfully!
) else (
    echo Build failed with error code %ERRORLEVEL%
)

pause 

