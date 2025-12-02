# Vector Studio Build Script for Windows
# Automatically installs dependencies if missing

param(
    [switch]$Clean,
    [switch]$Release,
    [switch]$Debug,
    [switch]$NoPython,
    [switch]$NoTests,
    [switch]$GPU,
    [switch]$AVX512,
    [switch]$CheckOnly,
    [switch]$AutoInstall
)

$ErrorActionPreference = "Stop"

# Colors
function Write-Header { param($msg) Write-Host "`n=== $msg ===" -ForegroundColor Cyan }
function Write-Success { param($msg) Write-Host "[OK] $msg" -ForegroundColor Green }
function Write-Warning { param($msg) Write-Host "[WARN] $msg" -ForegroundColor Yellow }
function Write-Err { param($msg) Write-Host "[ERROR] $msg" -ForegroundColor Red }
function Write-Info { param($msg) Write-Host "[INFO] $msg" -ForegroundColor Blue }

# ============================================================================
# Dependency Installation Functions
# ============================================================================
function Install-CMake {
    Write-Info "Installing CMake via winget..."
    $result = winget install Kitware.CMake --accept-source-agreements --accept-package-agreements 2>&1
    if ($LASTEXITCODE -eq 0) {
        # Refresh PATH
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
        Write-Success "CMake installed successfully"
        return $true
    } else {
        Write-Err "Failed to install CMake. Please install manually from https://cmake.org/download/"
        return $false
    }
}

function Install-Ninja {
    Write-Info "Installing Ninja via winget..."
    $result = winget install Ninja-build.Ninja --accept-source-agreements --accept-package-agreements 2>&1
    if ($LASTEXITCODE -eq 0) {
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")
        Write-Success "Ninja installed successfully"
        return $true
    } else {
        Write-Warning "Failed to install Ninja (optional). Build will use default generator."
        return $false
    }
}

function Test-VisualStudio {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -property installationPath 2>$null
        if ($vsPath) {
            return $true
        }
    }
    # Check for Build Tools
    $btPath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools"
    if (Test-Path $btPath) {
        return $true
    }
    return $false
}

function Install-VSBuildTools {
    Write-Info "Installing Visual Studio Build Tools 2022..."
    $result = winget install Microsoft.VisualStudio.2022.BuildTools --accept-source-agreements --accept-package-agreements 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Success "VS Build Tools installed (may require restart)"
        Write-Warning "You may need to run 'Visual Studio Installer' to add C++ workload"
        return $true
    } else {
        Write-Err "Failed to install VS Build Tools"
        return $false
    }
}

function Test-Dependencies {
    Write-Header "Checking Dependencies"
    
    $status = @{
        cmake = $false
        ninja = $false
        vs = $false
        python = $false
    }
    
    # CMake
    $cmake = Get-Command cmake -ErrorAction SilentlyContinue
    if ($cmake) {
        $version = (& cmake --version | Select-Object -First 1)
        Write-Success "CMake: $version"
        $status.cmake = $true
    } else {
        Write-Err "CMake not found"
        if ($AutoInstall) {
            $status.cmake = Install-CMake
        }
    }
    
    # Ninja (optional)
    $ninja = Get-Command ninja -ErrorAction SilentlyContinue
    if ($ninja) {
        $version = & ninja --version
        Write-Success "Ninja: $version (faster builds)"
        $status.ninja = $true
    } else {
        Write-Info "Ninja not found (optional, will use VS generator)"
        if ($AutoInstall) {
            $status.ninja = Install-Ninja
        }
    }
    
    # Visual Studio / Build Tools
    if (Test-VisualStudio) {
        $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path $vswhere) {
            $vsName = & $vswhere -latest -property displayName 2>$null
            Write-Success "Visual Studio: $vsName"
        } else {
            Write-Success "Visual Studio Build Tools found"
        }
        $status.vs = $true
    } else {
        Write-Err "Visual Studio or Build Tools not found"
        if ($AutoInstall) {
            $status.vs = Install-VSBuildTools
        }
    }
    
    # Python (optional for bindings)
    $python = Get-Command python -ErrorAction SilentlyContinue
    if ($python) {
        $version = & python --version 2>&1
        Write-Success "Python: $version"
        $status.python = $true
    } else {
        Write-Info "Python not found (optional for bindings)"
    }
    
    return $status
}

# ============================================================================
# Banner
# ============================================================================
Write-Host @"

 ██╗   ██╗███████╗ ██████╗████████╗ ██████╗ ██████╗ 
 ██║   ██║██╔════╝██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗
 ██║   ██║█████╗  ██║        ██║   ██║   ██║██████╔╝
 ╚██╗ ██╔╝██╔══╝  ██║        ██║   ██║   ██║██╔══██╗
  ╚████╔╝ ███████╗╚██████╗   ██║   ╚██████╔╝██║  ██║
   ╚═══╝  ╚══════╝ ╚═════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝
   ███████╗████████╗██╗   ██╗██████╗ ██╗ ██████╗ 
   ██╔════╝╚══██╔══╝██║   ██║██╔══██╗██║██╔═══██╗
   ███████╗   ██║   ██║   ██║██║  ██║██║██║   ██║
   ╚════██║   ██║   ██║   ██║██║  ██║██║██║   ██║
   ███████║   ██║   ╚██████╔╝██████╔╝██║╚██████╔╝
   ╚══════╝   ╚═╝    ╚═════╝ ╚═════╝ ╚═╝ ╚═════╝ 

                     Build System

"@ -ForegroundColor Yellow

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $ProjectRoot "build"

# Determine build type
$BuildType = if ($Release) { "Release" } elseif ($Debug) { "Debug" } else { "Release" }

# ============================================================================
# Check Dependencies First
# ============================================================================
$deps = Test-Dependencies

if ($CheckOnly) {
    Write-Header "Dependency Check Complete"
    if ($deps.cmake -and $deps.vs) {
        Write-Success "All required dependencies are installed!"
        exit 0
    } else {
        Write-Err "Missing required dependencies. Run with -AutoInstall to install them."
        exit 1
    }
}

# Verify required dependencies
if (-not $deps.cmake) {
    Write-Err "CMake is required. Run with -AutoInstall or install manually:"
    Write-Host "  winget install Kitware.CMake"
    exit 1
}

if (-not $deps.vs) {
    Write-Err "Visual Studio or Build Tools required. Run with -AutoInstall or install:"
    Write-Host "  winget install Microsoft.VisualStudio.2022.BuildTools"
    exit 1
}

Write-Header "Configuration"
Write-Host "  Project Root:  $ProjectRoot"
Write-Host "  Build Dir:     $BuildDir"
Write-Host "  Build Type:    $BuildType"
Write-Host "  Python:        $(-not $NoPython)"
Write-Host "  Tests:         $(-not $NoTests)"
Write-Host "  GPU Support:   $GPU"
Write-Host "  AVX-512:       $AVX512"

# Clean build
if ($Clean -and (Test-Path $BuildDir)) {
    Write-Header "Cleaning build directory"
    Remove-Item -Recurse -Force $BuildDir
    Write-Success "Clean complete"
}

# Create build directory
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# Configure
Write-Header "Configuring CMake"

$CMakeArgs = @(
    "-S", $ProjectRoot,
    "-B", $BuildDir,
    "-G", "Visual Studio 17 2022",
    "-A", "x64",
    "-DCMAKE_BUILD_TYPE=$BuildType"
)

if ($NoPython) {
    $CMakeArgs += "-DVDB_BUILD_PYTHON=OFF"
}

if ($NoTests) {
    $CMakeArgs += "-DVDB_BUILD_TESTS=OFF"
}

if ($GPU) {
    $CMakeArgs += "-DVDB_ENABLE_GPU=ON"
}

if ($AVX512) {
    $CMakeArgs += "-DVDB_USE_AVX512=ON"
}

& cmake @CMakeArgs

if ($LASTEXITCODE -ne 0) {
    Write-Error "CMake configuration failed!"
    exit 1
}

Write-Success "Configuration complete"

# Build
Write-Header "Building"

& cmake --build $BuildDir --config $BuildType --parallel

if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed!"
    exit 1
}

Write-Success "Build complete"

# Run tests
if (-not $NoTests) {
    Write-Header "Running Tests"
    
    Push-Location $BuildDir
    & ctest -C $BuildType --output-on-failure
    $TestResult = $LASTEXITCODE
    Pop-Location
    
    if ($TestResult -ne 0) {
        Write-Warning "Some tests failed"
    } else {
        Write-Success "All tests passed"
    }
}

# Summary
Write-Header "Build Summary"

$CliPath = Join-Path $BuildDir $BuildType "vdb_cli.exe"
$LibPath = Join-Path $BuildDir $BuildType "vdb_core.lib"

if (Test-Path $CliPath) {
    Write-Success "  CLI:     $CliPath"
}

if (Test-Path $LibPath) {
    Write-Success "  Library: $LibPath"
}

# Find Python module
$PyModule = Get-ChildItem -Path $BuildDir -Filter "pyvdb*.pyd" -Recurse | Select-Object -First 1
if ($PyModule) {
    Write-Success "  Python:  $($PyModule.FullName)"
}

Write-Host "`nDone!" -ForegroundColor Green
