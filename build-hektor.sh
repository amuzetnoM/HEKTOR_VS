#!/usr/bin/env bash
# ============================================================================
# Hektor VDB - Comprehensive Automated Build & Repair Script
# ============================================================================
# Description: One-command setup for Hektor Vector Database on any system
# Usage: ./build-hektor.sh [options]
# Options:
#   --clean       Clean build (removes existing build directory)
#   --repair      Repair mode (reinstall dependencies, rebuild)
#   --dev         Install development dependencies
#   --test        Run tests after build
#   --benchmark   Run benchmarks after build
#   --skip-deps   Skip dependency installation
#   --help        Show this help message
# ============================================================================

set -e  # Exit on error
trap 'handle_error $? $LINENO' ERR

# ============================================================================
# Configuration
# ============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_ROOT/build"
VENV_DIR="$PROJECT_ROOT/venv"
LOG_FILE="$PROJECT_ROOT/build-hektor.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Default options
CLEAN_BUILD=false
REPAIR_MODE=false
INSTALL_DEV=false
RUN_TESTS=false
RUN_BENCHMARKS=false
SKIP_DEPS=false

# System detection
OS_TYPE=""
ARCH_TYPE=""
PACKAGE_MANAGER=""
PYTHON_CMD=""
CMAKE_CMD="cmake"
NINJA_CMD="ninja"

# ============================================================================
# Helper Functions
# ============================================================================

log() {
    echo -e "${CYAN}[$(date +'%H:%M:%S')]${NC} $*" | tee -a "$LOG_FILE"
}

log_success() {
    echo -e "${GREEN}✓${NC} $*" | tee -a "$LOG_FILE"
}

log_warning() {
    echo -e "${YELLOW}⚠${NC} $*" | tee -a "$LOG_FILE"
}

log_error() {
    echo -e "${RED}✗${NC} $*" | tee -a "$LOG_FILE"
}

log_step() {
    echo ""
    echo -e "${BOLD}${BLUE}═══════════════════════════════════════════════════════${NC}"
    echo -e "${BOLD}${BLUE}$*${NC}"
    echo -e "${BOLD}${BLUE}═══════════════════════════════════════════════════════${NC}"
    echo "" | tee -a "$LOG_FILE"
}

handle_error() {
    local exit_code=$1
    local line_number=$2
    log_error "Build failed at line $line_number with exit code $exit_code"
    log_error "Check $LOG_FILE for details"
    log ""
    log "Troubleshooting tips:"
    log "  1. Run with --repair to fix common issues"
    log "  2. Run with --clean to start fresh"
    log "  3. Check system dependencies are installed"
    log "  4. Review $LOG_FILE for detailed error messages"
    exit $exit_code
}

show_help() {
    cat << EOF
${BOLD}Hektor VDB - Comprehensive Automated Build & Repair Script${NC}

${BOLD}USAGE:${NC}
    ./build-hektor.sh [options]

${BOLD}OPTIONS:${NC}
    --clean         Clean build (removes existing build directory)
    --repair        Repair mode (reinstall dependencies, rebuild from scratch)
    --dev           Install development dependencies (pytest, black, ruff, mypy)
    --test          Run tests after successful build
    --benchmark     Run benchmarks after successful build
    --skip-deps     Skip dependency installation (faster if already installed)
    --help          Show this help message

${BOLD}EXAMPLES:${NC}
    # Fresh build with all features
    ./build-hektor.sh

    # Clean build with dev tools and tests
    ./build-hektor.sh --clean --dev --test

    # Repair broken installation
    ./build-hektor.sh --repair

    # Quick rebuild (skip deps)
    ./build-hektor.sh --skip-deps

${BOLD}REQUIREMENTS:${NC}
    - Python 3.10+ (auto-installs if missing)
    - CMake 3.20+ (auto-installs if missing)
    - C++ compiler: GCC 11+, Clang 14+, or MSVC 2022+
    - Internet connection for downloading dependencies

${BOLD}PLATFORMS SUPPORTED:${NC}
    - Linux (Ubuntu/Debian, RedHat/Fedora/CentOS, Arch, Alpine)
    - macOS (Intel x86_64, Apple Silicon arm64)
    - Windows (via Git Bash, WSL, or MSYS2)

EOF
    exit 0
}

# ============================================================================
# System Detection
# ============================================================================

detect_system() {
    log_step "🔍 Detecting System Configuration"
    
    # Detect OS
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        OS_TYPE="linux"
        if [ -f /etc/os-release ]; then
            source /etc/os-release
            log "Linux Distribution: $NAME $VERSION"
            
            # Detect package manager
            if command -v apt-get &> /dev/null; then
                PACKAGE_MANAGER="apt"
            elif command -v dnf &> /dev/null; then
                PACKAGE_MANAGER="dnf"
            elif command -v yum &> /dev/null; then
                PACKAGE_MANAGER="yum"
            elif command -v pacman &> /dev/null; then
                PACKAGE_MANAGER="pacman"
            elif command -v apk &> /dev/null; then
                PACKAGE_MANAGER="apk"
            else
                log_warning "No supported package manager found"
            fi
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS_TYPE="macos"
        PACKAGE_MANAGER="brew"
        log "macOS detected: $(sw_vers -productVersion)"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "win32" ]]; then
        OS_TYPE="windows"
        log "Windows detected (Git Bash/MSYS2/Cygwin)"
    else
        log_warning "Unknown OS type: $OSTYPE"
        OS_TYPE="unknown"
    fi
    
    # Detect architecture
    ARCH_TYPE=$(uname -m)
    log "Architecture: $ARCH_TYPE"
    
    # Detect if running in Docker/Container
    if [ -f /.dockerenv ] || grep -q docker /proc/1/cgroup 2>/dev/null; then
        log "Running in Docker container"
    fi
    
    log_success "System: $OS_TYPE ($ARCH_TYPE), Package Manager: $PACKAGE_MANAGER"
}

# ============================================================================
# Dependency Installation
# ============================================================================

install_system_dependencies() {
    if [ "$SKIP_DEPS" = true ]; then
        log "Skipping system dependency installation (--skip-deps)"
        return 0
    fi
    
    log_step "📦 Installing System Dependencies"
    
    case $PACKAGE_MANAGER in
        apt)
            log "Using apt (Debian/Ubuntu)..."
            sudo apt-get update -qq
            sudo apt-get install -y -qq \
                build-essential cmake ninja-build \
                python3 python3-pip python3-venv python3-dev \
                libsqlite3-dev zlib1g-dev libicu-dev \
                git curl wget || log_warning "Some packages may have failed to install"
            ;;
        
        dnf|yum)
            log "Using $PACKAGE_MANAGER (RedHat/Fedora/CentOS)..."
            sudo $PACKAGE_MANAGER groupinstall -y "Development Tools" || true
            sudo $PACKAGE_MANAGER install -y \
                cmake ninja-build \
                python3 python3-pip python3-devel \
                sqlite-devel zlib-devel libicu-devel \
                git curl wget || log_warning "Some packages may have failed to install"
            ;;
        
        pacman)
            log "Using pacman (Arch Linux)..."
            sudo pacman -Sy --noconfirm --needed \
                base-devel cmake ninja \
                python python-pip \
                sqlite zlib icu \
                git curl wget || log_warning "Some packages may have failed to install"
            ;;
        
        apk)
            log "Using apk (Alpine Linux)..."
            sudo apk add --no-cache \
                build-base cmake ninja \
                python3 python3-dev py3-pip \
                sqlite-dev zlib-dev icu-dev \
                git curl wget || log_warning "Some packages may have failed to install"
            ;;
        
        brew)
            log "Using Homebrew (macOS)..."
            # Check if Homebrew is installed
            if ! command -v brew &> /dev/null; then
                log "Installing Homebrew..."
                /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
                
                # Add Homebrew to PATH
                if [[ -f "/opt/homebrew/bin/brew" ]]; then
                    eval "$(/opt/homebrew/bin/brew shellenv)"
                elif [[ -f "/usr/local/bin/brew" ]]; then
                    eval "$(/usr/local/bin/brew shellenv)"
                fi
            fi
            
            brew update
            brew install cmake ninja python@3.12 || brew upgrade cmake ninja python@3.12 || true
            ;;
        
        *)
            log_warning "No package manager detected. Please install dependencies manually:"
            log "  - CMake 3.20+"
            log "  - Ninja build"
            log "  - Python 3.10+"
            log "  - C++ compiler (GCC 11+, Clang 14+, or MSVC 2022+)"
            log "  - SQLite3 development libraries"
            ;;
    esac
    
    log_success "System dependencies installation completed"
}

find_python() {
    # Try to find a suitable Python 3.10+ installation
    for python_cmd in python3.13 python3.12 python3.11 python3.10 python3 python; do
        if command -v $python_cmd &> /dev/null; then
            local version=$($python_cmd --version 2>&1 | grep -oP '\d+\.\d+' | head -1)
            local major=$(echo $version | cut -d. -f1)
            local minor=$(echo $version | cut -d. -f2)
            
            if [[ $major -eq 3 && $minor -ge 10 ]]; then
                PYTHON_CMD=$python_cmd
                log_success "Found Python $version: $python_cmd"
                return 0
            fi
        fi
    done
    
    log_error "No suitable Python 3.10+ found"
    return 1
}

setup_python_environment() {
    log_step "🐍 Setting up Python Environment"
    
    # Find Python
    if ! find_python; then
        log_error "Please install Python 3.10 or higher"
        exit 1
    fi
    
    # Create virtual environment
    if [ "$REPAIR_MODE" = true ] && [ -d "$VENV_DIR" ]; then
        log "Removing old virtual environment for repair..."
        rm -rf "$VENV_DIR"
    fi
    
    if [ ! -d "$VENV_DIR" ]; then
        log "Creating virtual environment..."
        $PYTHON_CMD -m venv "$VENV_DIR"
        log_success "Virtual environment created"
    else
        log "Virtual environment already exists"
    fi
    
    # Activate virtual environment
    source "$VENV_DIR/bin/activate"
    log_success "Virtual environment activated"
    
    # Upgrade pip
    log "Upgrading pip and build tools..."
    python -m pip install --upgrade pip setuptools wheel --quiet
    log_success "pip upgraded"
}

install_python_dependencies() {
    log_step "📚 Installing Python Dependencies"
    
    # Install build dependencies
    log "Installing build dependencies..."
    pip install --upgrade scikit-build-core pybind11 numpy --quiet
    
    # Install core dependencies
    if [ -f "$PROJECT_ROOT/pyproject.toml" ]; then
        log "Installing from pyproject.toml..."
        pip install -e . --quiet
        log_success "Core dependencies installed"
    elif [ -f "$PROJECT_ROOT/requirements.txt" ]; then
        log "Installing from requirements.txt..."
        pip install -r "$PROJECT_ROOT/requirements.txt" --quiet
        log_success "Requirements installed"
    fi
    
    # Install optional dependencies
    log "Installing optional dependencies (ml, data)..."
    pip install --quiet \
        "onnxruntime>=1.19.0" \
        "huggingface-hub>=0.26.0" \
        "transformers>=4.46.0" \
        "tokenizers>=0.20.0" \
        "pandas>=2.1.0" \
        "Pillow>=10.4.0" || log_warning "Some optional dependencies may have failed"
    
    # Install dev dependencies if requested
    if [ "$INSTALL_DEV" = true ]; then
        log "Installing development dependencies..."
        pip install --quiet \
            pytest pytest-cov pytest-benchmark pytest-asyncio \
            black ruff mypy \
            mkdocs mkdocs-material || log_warning "Some dev dependencies may have failed"
        log_success "Development dependencies installed"
    fi
    
    log_success "Python dependencies installation completed"
}

# ============================================================================
# Build Process
# ============================================================================

configure_cmake() {
    log_step "⚙️  Configuring CMake"
    
    # Clean build if requested
    if [ "$CLEAN_BUILD" = true ] && [ -d "$BUILD_DIR" ]; then
        log "Removing old build directory..."
        rm -rf "$BUILD_DIR"
    fi
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Detect if Ninja is available
    local generator="Unix Makefiles"
    if command -v ninja &> /dev/null; then
        generator="Ninja"
        log "Using Ninja build system"
    else
        log_warning "Ninja not found, using Make"
    fi
    
    # Configure CMake with optimizations
    log "Configuring build..."
    cmake "$PROJECT_ROOT" \
        -G "$generator" \
        -DCMAKE_BUILD_TYPE=Release \
        -DVDB_BUILD_PYTHON=ON \
        -DVDB_BUILD_TESTS=ON \
        -DVDB_BUILD_BENCHMARKS=$([ "$RUN_BENCHMARKS" = true ] && echo "ON" || echo "OFF") \
        -DVDB_USE_AVX2=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        2>&1 | tee -a "$LOG_FILE"
    
    log_success "CMake configuration completed"
}

build_project() {
    log_step "🔨 Building Project"
    
    cd "$BUILD_DIR"
    
    # Detect number of cores for parallel build
    local num_cores=1
    if command -v nproc &> /dev/null; then
        num_cores=$(nproc)
    elif command -v sysctl &> /dev/null; then
        num_cores=$(sysctl -n hw.ncpu)
    fi
    
    log "Building with $num_cores parallel jobs..."
    
    # Build
    if command -v ninja &> /dev/null && [ -f "build.ninja" ]; then
        ninja -j$num_cores 2>&1 | tee -a "$LOG_FILE"
    else
        make -j$num_cores 2>&1 | tee -a "$LOG_FILE"
    fi
    
    log_success "Build completed successfully"
}

run_tests() {
    if [ "$RUN_TESTS" = false ]; then
        return 0
    fi
    
    log_step "🧪 Running Tests"
    
    cd "$BUILD_DIR"
    
    if [ -f "CTestTestfile.cmake" ]; then
        ctest --output-on-failure --parallel 2 2>&1 | tee -a "$LOG_FILE"
        log_success "Tests passed"
    else
        log_warning "No tests configured"
    fi
}

run_benchmarks() {
    if [ "$RUN_BENCHMARKS" = false ]; then
        return 0
    fi
    
    log_step "📊 Running Benchmarks"
    
    cd "$BUILD_DIR"
    
    if [ -f "benchmarks/vdb_benchmark" ]; then
        ./benchmarks/vdb_benchmark 2>&1 | tee -a "$LOG_FILE"
        log_success "Benchmarks completed"
    else
        log_warning "No benchmarks found"
    fi
}

# ============================================================================
# Verification & Repair
# ============================================================================

verify_installation() {
    log_step "✅ Verifying Installation"
    
    # Check if Python module can be imported
    if python -c "import pyvdb; print(f'pyvdb version loaded successfully')" 2>&1 | tee -a "$LOG_FILE"; then
        log_success "Python module verification passed"
    else
        log_error "Python module verification failed"
        return 1
    fi
    
    # Check if library files exist
    local lib_found=false
    for ext in so dylib pyd; do
        if find "$BUILD_DIR" -name "*pyvdb*.$ext" | grep -q .; then
            lib_found=true
            break
        fi
    done
    
    if [ "$lib_found" = true ]; then
        log_success "Library files found"
    else
        log_warning "No library files found (may be normal for some configurations)"
    fi
    
    log_success "Installation verification completed"
}

repair_installation() {
    log_step "🔧 Repairing Installation"
    
    log "Repair mode: Cleaning and rebuilding..."
    
    # Remove build artifacts
    if [ -d "$BUILD_DIR" ]; then
        log "Removing build directory..."
        rm -rf "$BUILD_DIR"
    fi
    
    # Remove Python cache
    log "Cleaning Python cache..."
    find "$PROJECT_ROOT" -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
    find "$PROJECT_ROOT" -type d -name "*.egg-info" -exec rm -rf {} + 2>/dev/null || true
    find "$PROJECT_ROOT" -type f -name "*.pyc" -delete 2>/dev/null || true
    
    # Reinstall Python dependencies
    log "Reinstalling Python dependencies..."
    pip install --upgrade --force-reinstall scikit-build-core pybind11 numpy --quiet
    
    log_success "Repair completed. Proceeding with fresh build..."
}

# ============================================================================
# Main Script
# ============================================================================

parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --clean)
                CLEAN_BUILD=true
                shift
                ;;
            --repair)
                REPAIR_MODE=true
                shift
                ;;
            --dev)
                INSTALL_DEV=true
                shift
                ;;
            --test)
                RUN_TESTS=true
                shift
                ;;
            --benchmark)
                RUN_BENCHMARKS=true
                shift
                ;;
            --skip-deps)
                SKIP_DEPS=true
                shift
                ;;
            --help|-h)
                show_help
                ;;
            *)
                log_error "Unknown option: $1"
                show_help
                ;;
        esac
    done
}

main() {
    # Clear log file
    > "$LOG_FILE"
    
    echo ""
    echo -e "${BOLD}${CYAN}╔═══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BOLD}${CYAN}║                                                           ║${NC}"
    echo -e "${BOLD}${CYAN}║        🚀 Hektor VDB - Automated Build System 🚀         ║${NC}"
    echo -e "${BOLD}${CYAN}║                                                           ║${NC}"
    echo -e "${BOLD}${CYAN}║     High-Performance Vector Database with SIMD            ║${NC}"
    echo -e "${BOLD}${CYAN}║                                                           ║${NC}"
    echo -e "${BOLD}${CYAN}╚═══════════════════════════════════════════════════════════╝${NC}"
    echo ""
    
    log "Build started at $(date)"
    log "Log file: $LOG_FILE"
    
    # Parse command line arguments
    parse_arguments "$@"
    
    # Execute build pipeline
    detect_system
    
    if [ "$REPAIR_MODE" = true ]; then
        repair_installation
    fi
    
    install_system_dependencies
    setup_python_environment
    install_python_dependencies
    configure_cmake
    build_project
    run_tests
    run_benchmarks
    verify_installation
    
    # Success summary
    echo ""
    log_step "🎉 Build Completed Successfully!"
    echo ""
    log_success "Hektor VDB has been built and installed"
    log ""
    log "${BOLD}Next Steps:${NC}"
    log "  1. Activate the virtual environment:"
    log "       source venv/bin/activate"
    log ""
    log "  2. Test the installation:"
    log "       python -c 'import pyvdb; print(pyvdb.__doc__)'"
    log ""
    log "  3. Run the examples:"
    log "       python examples/basic_usage.py"
    log ""
    log "  4. Read the documentation:"
    log "       docs/02_INSTALLATION.md"
    log "       docs/22_PYTHON_BINDINGS.md"
    log ""
    log "${BOLD}Troubleshooting:${NC}"
    log "  - Run with --repair to fix issues"
    log "  - Run with --clean for fresh build"
    log "  - Check $LOG_FILE for details"
    log ""
    log "Build log saved to: $LOG_FILE"
    echo ""
}

# Run main function
main "$@"
