# Build script for gen3dart standalone OpenGL project
#
# Prerequisites:
#   1. CMake must be in PATH (or set CMAKE_PATH environment variable)
#   2. Visual Studio 2022 with C++ workload installed

# Get script directory
$SCRIPT_DIR = $PSScriptRoot
if (-not $SCRIPT_DIR) { $SCRIPT_DIR = Split-Path -Parent $MyInvocation.MyCommand.Path }

# CMake - use from PATH or environment variable
if ($env:CMAKE_PATH) {
    $CMAKE_PATH = $env:CMAKE_PATH
} else {
    $CMAKE_PATH = (Get-Command cmake -ErrorAction SilentlyContinue).Source
    if (-not $CMAKE_PATH) {
        Write-Host "Error: CMake not found in PATH. Install CMake or set CMAKE_PATH environment variable." -ForegroundColor Red
        exit 1
    }
}

$OUTPUT_DIR = "$SCRIPT_DIR\out"
$BIN_DIR = "$SCRIPT_DIR\bin"

Write-Host "Using CMake: $CMAKE_PATH" -ForegroundColor Gray
Write-Host ""

# Create output directories
New-Item -ItemType Directory -Force -Path $OUTPUT_DIR | Out-Null
New-Item -ItemType Directory -Force -Path $BIN_DIR | Out-Null

# Change to output directory
Push-Location $OUTPUT_DIR

# Configure CMake
Write-Host "Configuring CMake..." -ForegroundColor Cyan
& $CMAKE_PATH -G "Visual Studio 17 2022" -A x64 -T v143 $SCRIPT_DIR

if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    Pop-Location
    exit 1
}

# Build the kinetic_sculpture project
Write-Host "Building kinetic_sculpture..." -ForegroundColor Cyan
& $CMAKE_PATH --build . --config Debug --target kinetic_sculpture

if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    Pop-Location
    exit 1
}

Pop-Location

# Copy shader files to bin directory
Write-Host "Copying shader files..." -ForegroundColor Cyan
Copy-Item "$SCRIPT_DIR\src\kinetic_sculpture\kinetic_pillar.vs" -Destination $BIN_DIR -Force
Copy-Item "$SCRIPT_DIR\src\kinetic_sculpture\kinetic_pillar.fs" -Destination $BIN_DIR -Force
Copy-Item "$SCRIPT_DIR\src\kinetic_sculpture\kinetic_light.vs" -Destination $BIN_DIR -Force
Copy-Item "$SCRIPT_DIR\src\kinetic_sculpture\kinetic_light.fs" -Destination $BIN_DIR -Force

Write-Host "`nBuild complete!" -ForegroundColor Green
Write-Host "Executable: $BIN_DIR\kinetic_sculpture.exe" -ForegroundColor Yellow
Write-Host "`nTo run: cd '$BIN_DIR'; .\kinetic_sculpture.exe" -ForegroundColor Yellow
