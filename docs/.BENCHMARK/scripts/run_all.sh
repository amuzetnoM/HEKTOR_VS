#!/bin/bash
#
# HEKTOR Benchmark Suite - Master Execution Script
# 
# Runs all database and studio benchmarks in sequence and generates
# comprehensive reports.
#
# Version: 1.0.0
# Date: 2026-01-24
#

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Benchmark root directory
BENCHMARK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$BENCHMARK_DIR"

# Configuration
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
REPORT_DIR="$BENCHMARK_DIR/reports/run_$TIMESTAMP"
LOG_FILE="$REPORT_DIR/execution.log"

# Create report directory
mkdir -p "$REPORT_DIR"/{database,studio,summary}

# Logging function
log() {
    echo -e "${BLUE}[$(date +'%Y-%m-%d %H:%M:%S')]${NC} $1" | tee -a "$LOG_FILE"
}

log_success() {
    echo -e "${GREEN}[$(date +'%Y-%m-%d %H:%M:%S')]${NC} $1" | tee -a "$LOG_FILE"
}

log_error() {
    echo -e "${RED}[$(date +'%Y-%m-%d %H:%M:%S')]${NC} $1" | tee -a "$LOG_FILE"
}

log_warning() {
    echo -e "${YELLOW}[$(date +'%Y-%m-%d %H:%M:%S')]${NC} $1" | tee -a "$LOG_FILE"
}

# Print header
echo ""
echo "═══════════════════════════════════════════════════════════════"
echo "  HEKTOR Comprehensive Benchmark Suite"
echo "  Version: 1.0.0"
echo "  Date: $(date +'%Y-%m-%d %H:%M:%S')"
echo "═══════════════════════════════════════════════════════════════"
echo ""

log "Starting benchmark suite execution"
log "Report directory: $REPORT_DIR"

# Parse command line arguments
RUN_DATABASE=true
RUN_STUDIO=true
GENERATE_REPORT=true
CI_MODE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --database-only)
            RUN_STUDIO=false
            shift
            ;;
        --studio-only)
            RUN_DATABASE=false
            shift
            ;;
        --no-report)
            GENERATE_REPORT=false
            shift
            ;;
        --ci-mode)
            CI_MODE=true
            shift
            ;;
        *)
            log_error "Unknown option: $1"
            exit 1
            ;;
    esac
done

# System information
log "Collecting system information..."
{
    echo "System Information:"
    echo "  OS: $(uname -s) $(uname -r)"
    echo "  CPU: $(lscpu | grep 'Model name' | cut -d':' -f2 | xargs)"
    echo "  CPU Cores: $(nproc)"
    echo "  Memory: $(free -h | awk '/^Mem:/ {print $2}')"
    echo "  Python: $(python3 --version 2>&1)"
    echo "  GCC: $(gcc --version 2>&1 | head -1)"
} | tee "$REPORT_DIR/system_info.txt"

# Run database benchmarks
if [ "$RUN_DATABASE" = true ]; then
    log "═══ Running Database Benchmarks ═══"
    
    if [ -x "$BENCHMARK_DIR/scripts/run_database.sh" ]; then
        log "Executing database benchmark script..."
        bash "$BENCHMARK_DIR/scripts/run_database.sh" "$REPORT_DIR/database" 2>&1 | tee -a "$LOG_FILE"
        
        if [ ${PIPESTATUS[0]} -eq 0 ]; then
            log_success "Database benchmarks completed successfully"
        else
            log_error "Database benchmarks failed"
            exit 1
        fi
    else
        log_warning "Database benchmark script not found or not executable"
    fi
else
    log "Skipping database benchmarks (--database-only not set)"
fi

# Run studio benchmarks
if [ "$RUN_STUDIO" = true ]; then
    log "═══ Running Studio Benchmarks ═══"
    
    if [ -x "$BENCHMARK_DIR/scripts/run_studio.sh" ]; then
        log "Executing studio benchmark script..."
        bash "$BENCHMARK_DIR/scripts/run_studio.sh" "$REPORT_DIR/studio" 2>&1 | tee -a "$LOG_FILE"
        
        if [ ${PIPESTATUS[0]} -eq 0 ]; then
            log_success "Studio benchmarks completed successfully"
        else
            log_error "Studio benchmarks failed"
            exit 1
        fi
    else
        log_warning "Studio benchmark script not found or not executable"
    fi
else
    log "Skipping studio benchmarks (--studio-only not set)"
fi

# Generate comprehensive report
if [ "$GENERATE_REPORT" = true ]; then
    log "═══ Generating Comprehensive Report ═══"
    
    if [ -f "$BENCHMARK_DIR/scripts/generate_report.py" ]; then
        log "Generating HTML and JSON reports..."
        python3 "$BENCHMARK_DIR/scripts/generate_report.py" \
            --report-dir "$REPORT_DIR" \
            --format html,json \
            --output "$REPORT_DIR/summary" 2>&1 | tee -a "$LOG_FILE"
        
        if [ ${PIPESTATUS[0]} -eq 0 ]; then
            log_success "Report generation completed successfully"
        else
            log_error "Report generation failed"
            exit 1
        fi
    else
        log_warning "Report generation script not found"
    fi
fi

# Summary
echo ""
echo "═══════════════════════════════════════════════════════════════"
log_success "Benchmark suite execution completed"
echo "═══════════════════════════════════════════════════════════════"
echo ""
log "Reports available at: $REPORT_DIR"
log "Execution log: $LOG_FILE"

if [ "$GENERATE_REPORT" = true ]; then
    if [ -f "$REPORT_DIR/summary/report.html" ]; then
        log "HTML Report: $REPORT_DIR/summary/report.html"
    fi
    if [ -f "$REPORT_DIR/summary/report.json" ]; then
        log "JSON Report: $REPORT_DIR/summary/report.json"
    fi
fi

echo ""
log_success "All benchmarks completed successfully! 🎉"
echo ""

exit 0
