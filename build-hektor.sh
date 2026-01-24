#!/usr/bin/env bash
# ============================================================================
# Hektor VDB - Automated Build & Repair Script
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
# Author: Ali A. Shakil / ARTIFACT VIRTUAL
# License: MIT
# Repository: https://github.com/amuzetnoM/hektor
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
VERSION="4.0.0"
BUILD_START_TIME=$(date +%s)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
WHITE='\033[1;37m'
BOLD='\033[1m'
DIM='\033[2m'
NC='\033[0m' # No Color

# Unicode characters for visuals
STAR="★"
ROCKET="🚀"
CHECK="✓"
CROSS="✗"
WARNING="⚠"
GEAR="⚙"
PACKAGE="📦"
PYTHON="🐍"
HAMMER="🔨"
TEST="🧪"
CHART="📊"
WRENCH="🔧"
SPARKLE="✨"
LIGHTNING="⚡"
FIRE="🔥"

# Default options
CLEAN_BUILD=false
REPAIR_MODE=false
INSTALL_DEV=false
RUN_TESTS=false
RUN_BENCHMARKS=false
SKIP_DEPS=false
QUIET_MODE=false

# System detection
OS_TYPE=""
ARCH_TYPE=""
PACKAGE_MANAGER=""
PYTHON_CMD=""
CMAKE_CMD="cmake"
NINJA_CMD="ninja"

# ============================================================================
# Visual Effects & Animation Functions
# ============================================================================

# Starfield animation - displays during long operations
starfield_animation() {
    local duration=${1:-3}
    local width=$(tput cols 2>/dev/null || echo 60)
    local height=8
    local stars=()
    local end_time=$((SECONDS + duration))
    
    # Initialize stars
    for ((i=0; i<30; i++)); do
        stars+=("$((RANDOM % width)):$((RANDOM % height)):$((RANDOM % 3))")
    done
    
    # Hide cursor
    tput civis 2>/dev/null || true
    
    while [ $SECONDS -lt $end_time ]; do
        # Clear animation area
        for ((y=0; y<height; y++)); do
            printf "\r%${width}s\n" ""
        done
        printf "\033[${height}A"
        
        # Draw stars
        for i in "${!stars[@]}"; do
            IFS=':' read -r x y brightness <<< "${stars[$i]}"
            
            # Move star
            x=$((x - 1 - brightness))
            if [ $x -lt 0 ]; then
                x=$((width - 1))
                y=$((RANDOM % height))
                brightness=$((RANDOM % 3))
            fi
            stars[$i]="$x:$y:$brightness"
            
            # Draw star based on brightness
            printf "\033[${y};${x}H"
            case $brightness in
                0) printf "${DIM}.${NC}" ;;
                1) printf "${WHITE}*${NC}" ;;
                2) printf "${BOLD}${WHITE}★${NC}" ;;
            esac
        done
        
        sleep 0.05
    done
    
    # Clear and show cursor
    for ((y=0; y<height; y++)); do
        printf "\r%${width}s\n" ""
    done
    printf "\033[${height}A"
    tput cnorm 2>/dev/null || true
}

# Progress bar with percentage
progress_bar() {
    local current=$1
    local total=$2
    local width=${3:-40}
    local label=${4:-"Progress"}
    
    local percent=$((current * 100 / total))
    local filled=$((current * width / total))
    local empty=$((width - filled))
    
    # Create the bar
    local bar=""
    for ((i=0; i<filled; i++)); do bar+="█"; done
    for ((i=0; i<empty; i++)); do bar+="░"; done
    
    # Color based on progress
    local color=$CYAN
    if [ $percent -ge 75 ]; then color=$GREEN
    elif [ $percent -ge 50 ]; then color=$YELLOW
    fi
    
    printf "\r${DIM}${label}:${NC} ${color}[${bar}]${NC} ${BOLD}${percent}%%${NC}"
}

# Spinner animation for ongoing tasks
spinner() {
    local pid=$1
    local message=${2:-"Working"}
    local spinners=("⠋" "⠙" "⠹" "⠸" "⠼" "⠴" "⠦" "⠧" "⠇" "⠏")
    local i=0
    
    tput civis 2>/dev/null || true
    
    while kill -0 $pid 2>/dev/null; do
        printf "\r${CYAN}${spinners[$i]}${NC} ${message}..."
        i=$(( (i + 1) % ${#spinners[@]} ))
        sleep 0.1
    done
    
    printf "\r%50s\r"
    tput cnorm 2>/dev/null || true
}

# Typing effect for important messages
type_effect() {
    local text="$1"
    local delay=${2:-0.03}
    
    for ((i=0; i<${#text}; i++)); do
        printf "%s" "${text:$i:1}"
        sleep $delay
    done
    echo ""
}

# Pulsing dot animation
pulse_dots() {
    local message=$1
    local duration=${2:-2}
    local end_time=$((SECONDS + duration))
    
    while [ $SECONDS -lt $end_time ]; do
        printf "\r${CYAN}${message}   ${NC}"
        sleep 0.3
        printf "\r${CYAN}${message}.  ${NC}"
        sleep 0.3
        printf "\r${CYAN}${message}.. ${NC}"
        sleep 0.3
        printf "\r${CYAN}${message}...${NC}"
        sleep 0.3
    done
    printf "\r%$((${#message} + 5))s\r"
}

# Matrix-style rain effect (brief)
matrix_effect() {
    local width=$(tput cols 2>/dev/null || echo 60)
    local chars="ヘクトルVDBアーティファクト01"
    local columns=()
    
    # Initialize columns
    for ((i=0; i<width; i+=3)); do
        columns+=($((RANDOM % 5)))
    done
    
    tput civis 2>/dev/null || true
    
    for ((frame=0; frame<15; frame++)); do
        printf "\r"
        local col=0
        for pos in "${columns[@]}"; do
            if [ $((RANDOM % 3)) -eq 0 ]; then
                printf "${GREEN}${chars:$((RANDOM % ${#chars})):1}${NC}"
            else
                printf " "
            fi
            printf "  "
            col=$((col + 1))
        done
        echo ""
        sleep 0.05
    done
    
    printf "\033[15A"
    for ((i=0; i<15; i++)); do
        printf "\r%${width}s\n" ""
    done
    printf "\033[15A"
    
    tput cnorm 2>/dev/null || true
}

# DNA helix animation
dna_helix() {
    local frames=20
    local width=40
    
    tput civis 2>/dev/null || true
    
    for ((f=0; f<frames; f++)); do
        printf "\r"
        for ((i=0; i<width; i++)); do
            local phase=$(( (i + f) % 8 ))
            case $phase in
                0|1) printf "${CYAN}╭${NC}" ;;
                2|3) printf "${BLUE}─${NC}" ;;
                4|5) printf "${CYAN}╮${NC}" ;;
                6|7) printf "${BLUE}─${NC}" ;;
            esac
        done
        sleep 0.08
    done
    printf "\r%${width}s\r"
    
    tput cnorm 2>/dev/null || true
}

# ============================================================================
# Helper Functions
# ============================================================================

log() {
    echo -e "${DIM}[$(date +'%H:%M:%S')]${NC} $*" | tee -a "$LOG_FILE"
}

log_success() {
    echo -e "${GREEN}${CHECK}${NC} $*" | tee -a "$LOG_FILE"
}

log_warning() {
    echo -e "${YELLOW}${WARNING}${NC} $*" | tee -a "$LOG_FILE"
}

log_error() {
    echo -e "${RED}${CROSS}${NC} $*" | tee -a "$LOG_FILE"
}

log_step() {
    echo ""
    echo -e "${BOLD}${MAGENTA}┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓${NC}"
    echo -e "${BOLD}${MAGENTA}┃${NC} ${BOLD}$*${NC}"
    echo -e "${BOLD}${MAGENTA}┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛${NC}"
    echo "" | tee -a "$LOG_FILE"
}

log_substep() {
    echo -e "  ${CYAN}▸${NC} $*" | tee -a "$LOG_FILE"
}

# Calculate elapsed time
elapsed_time() {
    local start=$1
    local end=$(date +%s)
    local diff=$((end - start))
    local mins=$((diff / 60))
    local secs=$((diff % 60))
    if [ $mins -gt 0 ]; then
        echo "${mins}m ${secs}s"
    else
        echo "${secs}s"
    fi
}

handle_error() {
    local exit_code=$1
    local line_number=$2
    echo ""
    echo -e "${RED}┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓${NC}"
    echo -e "${RED}┃${NC} ${BOLD}${RED}BUILD FAILED${NC}"
    echo -e "${RED}┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛${NC}"
    echo ""
    log_error "Build failed at line $line_number with exit code $exit_code"
    log_error "Check $LOG_FILE for details"
    echo ""
    echo -e "${YELLOW}${BOLD}Troubleshooting Tips:${NC}"
    echo -e "  ${CYAN}1.${NC} Run with ${BOLD}--repair${NC} to fix common issues"
    echo -e "  ${CYAN}2.${NC} Run with ${BOLD}--clean${NC} to start fresh"
    echo -e "  ${CYAN}3.${NC} Ensure C++23 compiler is installed (GCC 13+, Clang 16+)"
    echo -e "  ${CYAN}4.${NC} Review ${BOLD}$LOG_FILE${NC} for detailed error messages"
    echo ""
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
    - C++ compiler: GCC 13+, Clang 16+, or MSVC 2022 17.3+ (C++23 required)
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
        log_substep "Skipping system dependency installation (--skip-deps)"
        return 0
    fi
    
    log_step "${PACKAGE} Installing System Dependencies"
    
    echo -e "  ${DIM}This may take a few minutes...${NC}"
    echo ""
    
    case $PACKAGE_MANAGER in
        apt)
            log_substep "Using apt (Debian/Ubuntu)..."
            log_substep "Updating package lists..."
            sudo apt-get update -qq 2>&1 | tee -a "$LOG_FILE" &
            spinner $! "Updating package lists"
            
            log_substep "Installing build tools and libraries..."
            sudo apt-get install -y -qq \
                build-essential cmake ninja-build \
                python3 python3-pip python3-venv python3-dev \
                libsqlite3-dev zlib1g-dev libicu-dev \
                git curl wget 2>&1 | tee -a "$LOG_FILE" &
            spinner $! "Installing packages"
            log_success "apt packages installed"
            ;;
        
        dnf|yum)
            log_substep "Using $PACKAGE_MANAGER (RedHat/Fedora/CentOS)..."
            sudo $PACKAGE_MANAGER groupinstall -y "Development Tools" 2>&1 | tee -a "$LOG_FILE" &
            spinner $! "Installing development tools"
            
            sudo $PACKAGE_MANAGER install -y \
                cmake ninja-build \
                python3 python3-pip python3-devel \
                sqlite-devel zlib-devel libicu-devel \
                git curl wget 2>&1 | tee -a "$LOG_FILE" &
            spinner $! "Installing packages"
            log_success "$PACKAGE_MANAGER packages installed"
            ;;
        
        pacman)
            log_substep "Using pacman (Arch Linux)..."
            sudo pacman -Sy --noconfirm --needed \
                base-devel cmake ninja \
                python python-pip \
                sqlite zlib icu \
                git curl wget 2>&1 | tee -a "$LOG_FILE" &
            spinner $! "Installing packages"
            log_success "pacman packages installed"
            ;;
        
        apk)
            log_substep "Using apk (Alpine Linux)..."
            sudo apk add --no-cache \
                build-base cmake ninja \
                python3 python3-dev py3-pip \
                sqlite-dev zlib-dev icu-dev \
                git curl wget 2>&1 | tee -a "$LOG_FILE" &
            spinner $! "Installing packages"
            log_success "apk packages installed"
            ;;
        
        brew)
            log_substep "Using Homebrew (macOS)..."
            # Check if Homebrew is installed
            if ! command -v brew &> /dev/null; then
                log_substep "Installing Homebrew..."
                /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)" &
                spinner $! "Installing Homebrew"
                
                # Add Homebrew to PATH
                if [[ -f "/opt/homebrew/bin/brew" ]]; then
                    eval "$(/opt/homebrew/bin/brew shellenv)"
                elif [[ -f "/usr/local/bin/brew" ]]; then
                    eval "$(/usr/local/bin/brew shellenv)"
                fi
            fi
            
            brew update 2>&1 | tee -a "$LOG_FILE" &
            spinner $! "Updating Homebrew"
            brew install cmake ninja python@3.12 2>&1 | tee -a "$LOG_FILE" || brew upgrade cmake ninja python@3.12 2>&1 | tee -a "$LOG_FILE" || true
            log_success "Homebrew packages installed"
            ;;
        
        *)
            log_warning "No package manager detected. Please install dependencies manually:"
            echo -e "    ${DIM}- CMake 3.20+${NC}"
            echo -e "    ${DIM}- Ninja build${NC}"
            echo -e "    ${DIM}- Python 3.10+${NC}"
            echo -e "    ${DIM}- C++ compiler (GCC 13+, Clang 16+, or MSVC 2022 17.3+)${NC}"
            echo -e "    ${DIM}- SQLite3 development libraries${NC}"
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
    log_step "${PYTHON} Setting up Python Environment"
    
    # Find Python
    if ! find_python; then
        log_error "Please install Python 3.10 or higher"
        exit 1
    fi
    
    # Create virtual environment
    if [ "$REPAIR_MODE" = true ] && [ -d "$VENV_DIR" ]; then
        log_substep "Removing old virtual environment for repair..."
        rm -rf "$VENV_DIR"
    fi
    
    if [ ! -d "$VENV_DIR" ]; then
        log_substep "Creating virtual environment..."
        $PYTHON_CMD -m venv "$VENV_DIR" &
        spinner $! "Creating venv"
        log_success "Virtual environment created"
    else
        log_substep "Virtual environment already exists"
    fi
    
    # Activate virtual environment
    source "$VENV_DIR/bin/activate"
    log_success "Virtual environment activated"
    
    # Upgrade pip
    log_substep "Upgrading pip and build tools..."
    python -m pip install --upgrade pip setuptools wheel --quiet 2>&1 | tee -a "$LOG_FILE" &
    spinner $! "Upgrading pip"
    log_success "pip upgraded to latest version"
}

install_python_dependencies() {
    log_step "${PACKAGE} Installing Python Dependencies"
    
    echo -e "  ${DIM}Installing required packages...${NC}"
    echo ""
    
    # Install build dependencies
    log_substep "Installing build dependencies..."
    pip install --upgrade scikit-build-core pybind11 numpy --quiet 2>&1 | tee -a "$LOG_FILE" &
    spinner $! "scikit-build-core, pybind11, numpy"
    
    # Install core dependencies
    if [ -f "$PROJECT_ROOT/pyproject.toml" ]; then
        log_substep "Installing from pyproject.toml..."
        pip install -e . --quiet 2>&1 | tee -a "$LOG_FILE" &
        spinner $! "Installing hektor-vdb"
        log_success "Core dependencies installed"
    elif [ -f "$PROJECT_ROOT/requirements.txt" ]; then
        log_substep "Installing from requirements.txt..."
        pip install -r "$PROJECT_ROOT/requirements.txt" --quiet 2>&1 | tee -a "$LOG_FILE" &
        spinner $! "Installing requirements"
        log_success "Requirements installed"
    fi
    
    # Install optional dependencies
    log_substep "Installing optional ML dependencies..."
    pip install --quiet \
        "onnxruntime>=1.19.0" \
        "huggingface-hub>=0.26.0" \
        "transformers>=4.46.0" \
        "tokenizers>=0.20.0" \
        "pandas>=2.1.0" \
        "Pillow>=10.4.0" 2>&1 | tee -a "$LOG_FILE" &
    spinner $! "ML dependencies (onnxruntime, transformers...)"
    log_success "Optional dependencies installed"
    
    # Install dev dependencies if requested
    if [ "$INSTALL_DEV" = true ]; then
        log_substep "Installing development dependencies..."
        pip install --quiet \
            pytest pytest-cov pytest-benchmark pytest-asyncio \
            black ruff mypy \
            mkdocs mkdocs-material 2>&1 | tee -a "$LOG_FILE" &
        spinner $! "Dev tools (pytest, black, ruff, mypy...)"
        log_success "Development dependencies installed"
    fi
    
    log_success "Python dependencies installation completed ${CHECK}"
}

# ============================================================================
# Build Process
# ============================================================================

configure_cmake() {
    log_step "${GEAR} Configuring CMake"
    
    # Clean build if requested
    if [ "$CLEAN_BUILD" = true ] && [ -d "$BUILD_DIR" ]; then
        log_substep "Removing old build directory..."
        rm -rf "$BUILD_DIR"
    fi
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Detect if Ninja is available
    local generator="Unix Makefiles"
    if command -v ninja &> /dev/null; then
        generator="Ninja"
        log_substep "Using Ninja build system ${LIGHTNING}"
    else
        log_warning "Ninja not found, using Make (slower)"
    fi
    
    # Configure CMake with optimizations
    log_substep "Configuring CMake..."
    echo ""
    cmake "$PROJECT_ROOT" \
        -G "$generator" \
        -DCMAKE_BUILD_TYPE=Release \
        -DVDB_BUILD_PYTHON=ON \
        -DVDB_BUILD_TESTS=ON \
        -DVDB_BUILD_BENCHMARKS=$([ "$RUN_BENCHMARKS" = true ] && echo "ON" || echo "OFF") \
        -DVDB_USE_AVX2=ON \
        -DVDB_USE_LLAMA_CPP=OFF \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        2>&1 | tee -a "$LOG_FILE"
    
    log_success "CMake configuration completed"
}

build_project() {
    log_step "${HAMMER} Building Project"
    
    cd "$BUILD_DIR"
    
    # Detect number of cores for parallel build
    local num_cores=1
    if command -v nproc &> /dev/null; then
        num_cores=$(nproc)
    elif command -v sysctl &> /dev/null; then
        num_cores=$(sysctl -n hw.ncpu)
    fi
    
    log_substep "Building with ${BOLD}$num_cores${NC} parallel jobs..."
    echo -e "  ${DIM}This is the main compilation step - please wait...${NC}"
    echo ""
    
    # Show a building animation header
    echo -e "  ${CYAN}╭────────────────────────────────────────────────────────╮${NC}"
    echo -e "  ${CYAN}│${NC}  ${BOLD}Compiling C++23 source files...${NC}                      ${CYAN}│${NC}"
    echo -e "  ${CYAN}╰────────────────────────────────────────────────────────╯${NC}"
    echo ""
    
    # Build with progress output
    if command -v ninja &> /dev/null && [ -f "build.ninja" ]; then
        ninja -j$num_cores 2>&1 | while IFS= read -r line; do
            # Show only progress lines and errors
            if [[ "$line" =~ ^\[.*\] ]]; then
                printf "\r  ${GREEN}%s${NC}" "$line"
            elif [[ "$line" =~ error:|Error:|ERROR ]]; then
                echo -e "\n  ${RED}$line${NC}"
            fi
            echo "$line" >> "$LOG_FILE"
        done
        echo ""
    else
        make -j$num_cores 2>&1 | tee -a "$LOG_FILE"
    fi
    
    log_success "Build completed successfully ${SPARKLE}"
}

run_tests() {
    if [ "$RUN_TESTS" = false ]; then
        return 0
    fi
    
    log_step "${TEST} Running Tests"
    
    cd "$BUILD_DIR"
    
    if [ -f "CTestTestfile.cmake" ]; then
        log_substep "Executing test suite..."
        ctest --output-on-failure --parallel 2 2>&1 | tee -a "$LOG_FILE"
        log_success "All tests passed ${CHECK}"
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
    
    # Clear screen for fresh start
    clear 2>/dev/null || true
    
    # Epic banner
    echo ""
    echo -e "${BOLD}${CYAN}"
    cat << 'BANNER'
    ██╗  ██╗███████╗██╗  ██╗████████╗ ██████╗ ██████╗     ██╗   ██╗██████╗ ██████╗ 
    ██║  ██║██╔════╝██║ ██╔╝╚══██╔══╝██╔═══██╗██╔══██╗    ██║   ██║██╔══██╗██╔══██╗
    ███████║█████╗  █████╔╝    ██║   ██║   ██║██████╔╝    ██║   ██║██║  ██║██████╔╝
    ██╔══██║██╔══╝  ██╔═██╗    ██║   ██║   ██║██╔══██╗    ╚██╗ ██╔╝██║  ██║██╔══██╗
    ██║  ██║███████╗██║  ██╗   ██║   ╚██████╔╝██║  ██║     ╚████╔╝ ██████╔╝██████╔╝
    ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝      ╚═══╝  ╚═════╝ ╚═════╝ 
BANNER
    echo -e "${NC}"
    echo ""
    echo -e "    ${BOLD}${WHITE}High-Performance Vector Database with SIMD Optimization${NC}"
    echo -e "    ${DIM}Version ${VERSION} • C++23 • AVX2/AVX-512 • ONNX Embeddings${NC}"
    echo ""
    echo -e "    ${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "    ${DIM}Author: Ali A. Shakil • ARTIFACT VIRTUAL${NC}"
    echo -e "    ${DIM}Repository: https://github.com/amuzetnoM/hektor${NC}"
    echo -e "    ${MAGENTA}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo ""
    
    # Short starfield animation
    echo -e "    ${DIM}Initializing build system...${NC}"
    sleep 0.5
    dna_helix
    echo ""
    
    log "Build started at $(date)"
    log "Log file: $LOG_FILE"
    echo ""
    
    # Parse command line arguments
    parse_arguments "$@"
    
    # Show build configuration
    echo -e "${BOLD}${WHITE}Build Configuration:${NC}"
    echo -e "  ${CYAN}▸${NC} Clean Build:    $([ "$CLEAN_BUILD" = true ] && echo "${GREEN}Yes${NC}" || echo "${DIM}No${NC}")"
    echo -e "  ${CYAN}▸${NC} Repair Mode:    $([ "$REPAIR_MODE" = true ] && echo "${GREEN}Yes${NC}" || echo "${DIM}No${NC}")"
    echo -e "  ${CYAN}▸${NC} Dev Tools:      $([ "$INSTALL_DEV" = true ] && echo "${GREEN}Yes${NC}" || echo "${DIM}No${NC}")"
    echo -e "  ${CYAN}▸${NC} Run Tests:      $([ "$RUN_TESTS" = true ] && echo "${GREEN}Yes${NC}" || echo "${DIM}No${NC}")"
    echo -e "  ${CYAN}▸${NC} Benchmarks:     $([ "$RUN_BENCHMARKS" = true ] && echo "${GREEN}Yes${NC}" || echo "${DIM}No${NC}")"
    echo ""
    
    # Execute build pipeline with progress tracking
    local total_steps=8
    local current_step=0
    
    ((current_step++)); progress_bar $current_step $total_steps 50 "Overall Progress"; echo ""
    detect_system
    
    if [ "$REPAIR_MODE" = true ]; then
        ((current_step++)); progress_bar $current_step $total_steps 50 "Overall Progress"; echo ""
        repair_installation
    fi
    
    ((current_step++)); progress_bar $current_step $total_steps 50 "Overall Progress"; echo ""
    install_system_dependencies
    
    ((current_step++)); progress_bar $current_step $total_steps 50 "Overall Progress"; echo ""
    setup_python_environment
    
    ((current_step++)); progress_bar $current_step $total_steps 50 "Overall Progress"; echo ""
    install_python_dependencies
    
    ((current_step++)); progress_bar $current_step $total_steps 50 "Overall Progress"; echo ""
    configure_cmake
    
    ((current_step++)); progress_bar $current_step $total_steps 50 "Overall Progress"; echo ""
    build_project
    
    ((current_step++)); progress_bar $current_step $total_steps 50 "Overall Progress"; echo ""
    run_tests
    run_benchmarks
    verify_installation
    
    # Final progress
    progress_bar $total_steps $total_steps 50 "Overall Progress"; echo ""
    
    # Calculate build time
    local build_time=$(elapsed_time $BUILD_START_TIME)
    
    # Success summary with celebration
    echo ""
    echo -e "${GREEN}"
    cat << 'SUCCESS'
    ╔═══════════════════════════════════════════════════════════════════════╗
    ║                                                                       ║
    ║   ██████╗ ██╗   ██╗██╗██╗     ██████╗      ██████╗ ██╗  ██╗██╗        ║
    ║   ██╔══██╗██║   ██║██║██║     ██╔══██╗    ██╔═══██╗██║ ██╔╝██║        ║
    ║   ██████╔╝██║   ██║██║██║     ██║  ██║    ██║   ██║█████╔╝ ██║        ║
    ║   ██╔══██╗██║   ██║██║██║     ██║  ██║    ██║   ██║██╔═██╗ ╚═╝        ║
    ║   ██████╔╝╚██████╔╝██║███████╗██████╔╝    ╚██████╔╝██║  ██╗██╗        ║
    ║   ╚═════╝  ╚═════╝ ╚═╝╚══════╝╚═════╝      ╚═════╝ ╚═╝  ╚═╝╚═╝        ║
    ║                                                                       ║
    ╚═══════════════════════════════════════════════════════════════════════╝
SUCCESS
    echo -e "${NC}"
    
    echo -e "    ${SPARKLE} ${BOLD}Hektor VDB has been successfully built!${NC} ${SPARKLE}"
    echo -e "    ${DIM}Build completed in ${BOLD}${build_time}${NC}"
    echo ""
    
    # Next steps in a nice box
    echo -e "${CYAN}┌─────────────────────────────────────────────────────────────────────┐${NC}"
    echo -e "${CYAN}│${NC} ${BOLD}${WHITE}Next Steps:${NC}                                                        ${CYAN}│${NC}"
    echo -e "${CYAN}├─────────────────────────────────────────────────────────────────────┤${NC}"
    echo -e "${CYAN}│${NC}                                                                     ${CYAN}│${NC}"
    echo -e "${CYAN}│${NC}  ${GREEN}1.${NC} Activate the virtual environment:                               ${CYAN}│${NC}"
    echo -e "${CYAN}│${NC}     ${DIM}source venv/bin/activate${NC}                                        ${CYAN}│${NC}"
    echo -e "${CYAN}│${NC}                                                                     ${CYAN}│${NC}"
    echo -e "${CYAN}│${NC}  ${GREEN}2.${NC} Test the installation:                                          ${CYAN}│${NC}"
    echo -e "${CYAN}│${NC}     ${DIM}python -c 'import pyvdb; print(\"Success!\")'${NC}                     ${CYAN}│${NC}"
    echo -e "${CYAN}│${NC}                                                                     ${CYAN}│${NC}"
    echo -e "${CYAN}│${NC}  ${GREEN}3.${NC} Run an example:                                                  ${CYAN}│${NC}"
    echo -e "${CYAN}│${NC}     ${DIM}python examples/basic_usage.py${NC}                                   ${CYAN}│${NC}"
    echo -e "${CYAN}│${NC}                                                                     ${CYAN}│${NC}"
    echo -e "${CYAN}│${NC}  ${GREEN}4.${NC} Read the documentation:                                          ${CYAN}│${NC}"
    echo -e "${CYAN}│${NC}     ${DIM}docs/02_INSTALLATION.md${NC}                                          ${CYAN}│${NC}"
    echo -e "${CYAN}│${NC}     ${DIM}docs/22_PYTHON_BINDINGS.md${NC}                                       ${CYAN}│${NC}"
    echo -e "${CYAN}│${NC}                                                                     ${CYAN}│${NC}"
    echo -e "${CYAN}└─────────────────────────────────────────────────────────────────────┘${NC}"
    echo ""
    
    # Quick commands
    echo -e "${YELLOW}┌─────────────────────────────────────────────────────────────────────┐${NC}"
    echo -e "${YELLOW}│${NC} ${BOLD}${WHITE}Quick Commands:${NC}                                                    ${YELLOW}│${NC}"
    echo -e "${YELLOW}├─────────────────────────────────────────────────────────────────────┤${NC}"
    echo -e "${YELLOW}│${NC}  ${DIM}Repair installation:${NC}  ./build-hektor.sh --repair                  ${YELLOW}│${NC}"
    echo -e "${YELLOW}│${NC}  ${DIM}Clean rebuild:${NC}        ./build-hektor.sh --clean                   ${YELLOW}│${NC}"
    echo -e "${YELLOW}│${NC}  ${DIM}Run tests:${NC}            ./build-hektor.sh --test                    ${YELLOW}│${NC}"
    echo -e "${YELLOW}│${NC}  ${DIM}Install via pip:${NC}      pip install hektor-vdb                      ${YELLOW}│${NC}"
    echo -e "${YELLOW}└─────────────────────────────────────────────────────────────────────┘${NC}"
    echo ""
    
    log "Build log saved to: $LOG_FILE"
    echo ""
    
    # Final celebration animation
    echo -e "    ${FIRE} ${LIGHTNING} ${ROCKET} ${BOLD}Happy coding!${NC} ${ROCKET} ${LIGHTNING} ${FIRE}"
    echo ""
}

# Run main function
main "$@"
