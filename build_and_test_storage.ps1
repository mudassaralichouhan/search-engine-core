#!/usr/bin/env pwsh
# Storage Layer Build and Test Script for Search Engine
# Supports both Windows PowerShell and PowerShell Core

param(
    [Parameter(Position=0)]
    [ValidateSet("clean", "all", "mongodb", "redis", "content", "verbose", "quick", "help")]
    [string]$TestMode = "all",
    
    [switch]$SkipBuild,
    [switch]$SkipDependencyCheck,
    [switch]$Parallel,
    [switch]$Coverage,
    [string]$Configuration = "Debug",
    [int]$Timeout = 300
)

# Color functions for better output
function Write-ColorOutput([string]$Message, [string]$Color = "White") {
    switch ($Color) {
        "Red"     { Write-Host $Message -ForegroundColor Red }
        "Green"   { Write-Host $Message -ForegroundColor Green }
        "Yellow"  { Write-Host $Message -ForegroundColor Yellow }
        "Blue"    { Write-Host $Message -ForegroundColor Blue }
        "Cyan"    { Write-Host $Message -ForegroundColor Cyan }
        "Magenta" { Write-Host $Message -ForegroundColor Magenta }
        default   { Write-Host $Message -ForegroundColor White }
    }
}

function Write-Info([string]$Message) { Write-ColorOutput "[INFO] $Message" "Cyan" }
function Write-Success([string]$Message) { Write-ColorOutput "[SUCCESS] $Message" "Green" }
function Write-Warning([string]$Message) { Write-ColorOutput "[WARNING] $Message" "Yellow" }
function Write-Error([string]$Message) { Write-ColorOutput "[ERROR] $Message" "Red" }

function Show-Help {
    Write-ColorOutput @"
===============================================
  Search Engine - Storage Layer Build & Test
===============================================

USAGE:
  .\build_and_test_storage.ps1 [TestMode] [Options]

TEST MODES:
  clean      - Clean build directory and rebuild everything
  all        - Run all storage tests (default)
  mongodb    - Run only MongoDB storage tests
  redis      - Run only Redis storage tests
  content    - Run only content storage integration tests
  verbose    - Run all tests with verbose output
  quick      - Run essential tests only
  help       - Show this help message

OPTIONS:
  -SkipBuild            Skip the build phase
  -SkipDependencyCheck  Skip dependency verification
  -Parallel             Use parallel build (faster)
  -Coverage             Generate code coverage report (if available)
  -Configuration <type> Build configuration (Debug/Release) [default: Debug]
  -Timeout <seconds>    Test timeout in seconds [default: 300]

EXAMPLES:
  .\build_and_test_storage.ps1 all
  .\build_and_test_storage.ps1 mongodb -Parallel
  .\build_and_test_storage.ps1 clean -Configuration Release
  .\build_and_test_storage.ps1 content -SkipBuild -Verbose

PREREQUISITES:
  - Visual Studio 2019/2022 with C++ support
  - CMake 3.12 or higher
  - vcpkg package manager
  - MongoDB instance (for MongoDB tests)
  - Redis instance (for Redis tests)

===============================================
"@ "White"
}

if ($TestMode -eq "help") {
    Show-Help
    exit 0
}

# Header
Write-ColorOutput @"
===============================================
  Search Engine - Storage Layer Build & Test
===============================================
"@ "Blue"

$ErrorActionPreference = "Stop"
$StartTime = Get-Date

try {
    # Environment validation
    if (-not $SkipDependencyCheck) {
        Write-Info "Checking build environment..."
        
        # Check CMake
        try {
            $cmakeVersion = cmake --version | Select-Object -First 1
            Write-Success "CMake found: $cmakeVersion"
        }
        catch {
            Write-Error "CMake not found. Please install CMake and add it to PATH."
            exit 1
        }
        
        # Check Visual Studio
        $vsPath = @(
            "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat",
            "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat",
            "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat",
            "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat",
            "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvarsall.bat"
        ) | Where-Object { Test-Path $_ } | Select-Object -First 1
        
        if (-not $vsPath) {
            Write-Error "Visual Studio 2019/2022 not found. Please install Visual Studio with C++ support."
            exit 1
        }
        Write-Success "Visual Studio found: $vsPath"
        
        # Check vcpkg
        if (-not (Test-Path "vcpkg.json")) {
            Write-Error "vcpkg.json not found. Please ensure you're in the project root directory."
            exit 1
        }
        Write-Success "vcpkg manifest found"
    }
    
    # Create and navigate to build directory
    if (-not (Test-Path "build")) {
        Write-Info "Creating build directory..."
        New-Item -Type Directory -Name "build" | Out-Null
    }
    
    Push-Location "build"
    
    try {
        # Clean build if requested
        if ($TestMode -eq "clean") {
            Write-Info "Cleaning previous build..."
            Get-ChildItem -Force | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
            Write-Success "Build directory cleaned"
        }
        
        # Configure CMake
        if (-not $SkipBuild) {
            if (-not (Test-Path "CMakeCache.txt") -or $TestMode -eq "clean") {
                Write-Info "Configuring CMake for storage layer..."
                
                $cmakeArgs = @(
                    "..",
                    "-G", "Visual Studio 17 2022",
                    "-A", "x64",
                    "-DBUILD_TESTS=ON",
                    "-DCMAKE_BUILD_TYPE=$Configuration",
                    "-DVCPKG_TARGET_TRIPLET=x64-windows"
                )
                
                $configResult = & cmake $cmakeArgs
                if ($LASTEXITCODE -ne 0) {
                    Write-Warning "VS2022 configuration failed, trying VS2019..."
                    $cmakeArgs[2] = "Visual Studio 16 2019"
                    & cmake $cmakeArgs
                    if ($LASTEXITCODE -ne 0) {
                        Write-Error "CMake configuration failed. Check dependencies and VS installation."
                        exit 1
                    }
                }
                Write-Success "CMake configured successfully"
            }
            else {
                Write-Info "Using existing CMake configuration"
            }
            
            # Build storage components
            Write-Info "Building storage layer components..."
            
            $buildArgs = @(
                "--build", ".",
                "--config", $Configuration
            )
            
            if ($Parallel) {
                $buildArgs += "--parallel"
            }
            
            # Build main storage library first
            Write-Info "Building storage library..."
            $result = & cmake ($buildArgs + @("--target", "storage"))
            if ($LASTEXITCODE -ne 0) {
                Write-Error "Failed to build storage library"
                exit 1
            }
            Write-Success "Storage library built successfully"
            
            # Build individual storage components
            $storageTargets = @("MongoDBStorage", "ContentStorage")
            foreach ($target in $storageTargets) {
                Write-Info "Building $target..."
                $result = & cmake ($buildArgs + @("--target", $target))
                if ($LASTEXITCODE -ne 0) {
                    Write-Warning "Failed to build $target (continuing with available components)"
                }
                else {
                    Write-Success "$target built successfully"
                }
            }
            
            # Try to build RedisSearchStorage (may fail if Redis dependencies not available)
            Write-Info "Building RedisSearchStorage..."
            $result = & cmake ($buildArgs + @("--target", "RedisSearchStorage"))
            if ($LASTEXITCODE -ne 0) {
                Write-Warning "RedisSearchStorage build failed (Redis dependencies may not be available)"
            }
            else {
                Write-Success "RedisSearchStorage built successfully"
            }
            
            # Build test executables
            Write-Info "Building storage tests..."
            $testTargets = @("test_mongodb_storage", "test_redis_search_storage", "test_content_storage", "storage_tests")
            
            foreach ($target in $testTargets) {
                Write-Info "Building test: $target..."
                $result = & cmake ($buildArgs + @("--target", $target))
                if ($LASTEXITCODE -eq 0) {
                    Write-Success "$target built successfully"
                }
                else {
                    Write-Warning "Failed to build $target (may be skipped in tests)"
                }
            }
        }
        
        # Pre-test environment check
        Write-Info "Checking test environment..."
        
        # Check MongoDB
        $mongoRunning = Test-NetConnection -ComputerName "localhost" -Port 27017 -InformationLevel Quiet -WarningAction SilentlyContinue
        if ($mongoRunning) {
            Write-Success "MongoDB detected on port 27017"
        }
        else {
            Write-Warning "MongoDB not detected on port 27017. MongoDB tests may be skipped."
            Write-Info "To start MongoDB: net start mongodb (if service is installed)"
        }
        
        # Check Redis
        $redisRunning = Test-NetConnection -ComputerName "localhost" -Port 6379 -InformationLevel Quiet -WarningAction SilentlyContinue
        if ($redisRunning) {
            Write-Success "Redis detected on port 6379"
        }
        else {
            Write-Warning "Redis not detected on port 6379. Redis tests may be skipped."
            Write-Info "To start Redis: redis-server (if installed)"
        }
        
        # Set environment variables for tests
        $env:MONGODB_URI = "mongodb://localhost:27017"
        $env:REDIS_URI = "tcp://127.0.0.1:6379"
        $env:LOG_LEVEL = "INFO"
        
        # Run tests based on mode
        Write-Info "Running storage tests (mode: $TestMode)..."
        
        $ctestArgs = @(
            "-C", $Configuration,
            "--output-on-failure",
            "--timeout", $Timeout
        )
        
        if ($Coverage) {
            $ctestArgs += "--coverage"
        }
        
        switch ($TestMode) {
            "all" {
                Write-Info "Running all storage tests..."
                $ctestArgs += @("-L", "storage")
            }
            "mongodb" {
                Write-Info "Running MongoDB storage tests..."
                $ctestArgs += @("-R", "test_mongodb_storage")
            }
            "redis" {
                Write-Info "Running Redis storage tests..."
                $ctestArgs += @("-R", "test_redis_search_storage")
            }
            "content" {
                Write-Info "Running content storage integration tests..."
                $ctestArgs += @("-R", "test_content_storage")
            }
            "verbose" {
                Write-Info "Running all storage tests with verbose output..."
                $ctestArgs += @("-V", "-L", "storage")
            }
            "quick" {
                Write-Info "Running essential storage tests..."
                $ctestArgs += @("-L", "storage", "-E", ".*stress.*|.*performance.*")
            }
            default {
                Write-Info "Running specific test: $TestMode..."
                $ctestArgs += @("-R", $TestMode)
            }
        }
        
        # Execute tests
        $testResult = & ctest $ctestArgs
        $testExitCode = $LASTEXITCODE
        
        # Generate test report
        Write-Info "Generating test report..."
        & ctest -C $Configuration --output-junit test_results.xml -L storage 2>$null
        
        if (Test-Path "test_results.xml") {
            Write-Success "Test results saved to: build\test_results.xml"
        }
        
        # Test summary
        Write-Info "Test Summary:"
        Write-ColorOutput "=====================================" "Blue"
        & ctest -C $Configuration -N -L storage | Where-Object { $_ -match "Test #" }
        
        # Results
        if ($testExitCode -eq 0) {
            Write-Success "All specified tests passed!"
            $success = $true
        }
        else {
            Write-Error "Some tests failed or no tests were found."
            Write-Info "Check the output above for details."
            $success = $false
        }
        
    }
    finally {
        Pop-Location
    }
    
    # Final summary
    $EndTime = Get-Date
    $Duration = $EndTime - $StartTime
    
    Write-ColorOutput "=====================================" "Blue"
    Write-Info "Storage layer testing complete"
    Write-Info "Duration: $($Duration.ToString('mm\:ss'))"
    Write-Info "Build artifacts in: build\"
    Write-Info "Test results in: build\test_results.xml"
    
    if ($success) {
        Write-Success "All tests completed successfully!"
        exit 0
    }
    else {
        Write-Warning "Some tests failed. Check output for details."
        exit 1
    }
}
catch {
    Write-Error "Build/test process failed: $($_.Exception.Message)"
    Write-Error "Stack trace: $($_.ScriptStackTrace)"
    exit 1
}
finally {
    Write-ColorOutput @"
====================================
USAGE REMINDER:
  .\build_and_test_storage.ps1 [clean|all|mongodb|redis|content|verbose|quick|help] [options]
  
  Use -Help for detailed usage information
====================================
"@ "Gray"
} 