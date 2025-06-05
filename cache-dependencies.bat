@echo off
echo ========================================
echo vcpkg Dependency Pre-caching Script
echo ========================================
echo This script will download all dependencies to local cache for offline use
echo.

REM Set vcpkg path
set VCPKG_ROOT=D:\Projects\hatef.ir\vcpkg

REM Configure environment for caching
set VCPKG_DISABLE_METRICS=1

echo Checking vcpkg installation...
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo ERROR: vcpkg.exe not found at %VCPKG_ROOT%
    echo Please check your VCPKG_ROOT path
    pause
    exit /b 1
)

echo.
echo Pre-caching dependencies for x64-windows...
echo This may take a while on first run...
echo.

REM Pre-cache all dependencies by installing them (downloads sources and builds)
echo Installing dependencies from manifest...
"%VCPKG_ROOT%\vcpkg" install --triplet x64-windows

echo.
echo Downloading sources only for offline use...
"%VCPKG_ROOT%\vcpkg" install --triplet x64-windows --only-downloads

echo.
echo Installing to populate binary cache...
"%VCPKG_ROOT%\vcpkg" install --triplet x64-windows --only-binarycaching 2>nul || (
    echo Binary cache not available, doing full install...
    "%VCPKG_ROOT%\vcpkg" install --triplet x64-windows
)

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Failed to cache some dependencies
    echo Check the output above for details
    pause
    exit /b 1
)

echo.
echo ========================================
echo Pre-caching completed successfully!
echo ========================================
echo.
echo All dependencies have been downloaded to:
echo   Downloads: %VCPKG_ROOT%\downloads\
echo   Binary Cache: %VCPKG_ROOT%\archives\
echo.
echo You can now build offline using build.bat
echo.

pause 