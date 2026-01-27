#!/bin/bash
#
# HEKTOR Benchmark Suite - Quick Validation
# 
# Runs a quick validation test to ensure all components are working
#
# Version: 1.0.0
# Date: 2026-01-24
#

set -e

BENCHMARK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$BENCHMARK_DIR"

echo "═══════════════════════════════════════════════════════════"
echo "  HEKTOR Benchmark Suite - Quick Validation"
echo "═══════════════════════════════════════════════════════════"
echo ""

# Check Python
echo "[1/5] Checking Python environment..."
if command -v python3 &> /dev/null; then
    echo "  ✓ Python 3 found: $(python3 --version)"
else
    echo "  ✗ Python 3 not found"
    exit 1
fi

# Check dependencies
echo "[2/5] Checking Python dependencies..."
python3 -c "import numpy" 2>/dev/null && echo "  ✓ numpy installed" || echo "  ⚠ numpy not found (install with: pip install numpy)"
python3 -c "import psutil" 2>/dev/null && echo "  ✓ psutil installed" || echo "  ⚠ psutil not found (install with: pip install psutil)"

# Check scripts
echo "[3/5] Checking benchmark scripts..."
if [ -x "scripts/run_all.sh" ]; then
    echo "  ✓ run_all.sh is executable"
else
    echo "  ✗ run_all.sh not executable"
    exit 1
fi

if [ -f "scripts/generate_report.py" ]; then
    echo "  ✓ generate_report.py exists"
else
    echo "  ✗ generate_report.py not found"
    exit 1
fi

# Check configuration files
echo "[4/5] Checking configuration files..."
if [ -f "configs/database.yaml" ]; then
    echo "  ✓ database.yaml exists"
else
    echo "  ✗ database.yaml not found"
    exit 1
fi

if [ -f "configs/studio.yaml" ]; then
    echo "  ✓ studio.yaml exists"
else
    echo "  ✗ studio.yaml not found"
    exit 1
fi

# Run a quick test
echo "[5/5] Running quick validation test..."
echo "  Running 1M vector stress test (quick mode)..."
python3 database/stress_tests/test_1m_vectors.py \
    --vectors 1000 \
    --dimension 128 \
    --threads 2 \
    --duration 5 \
    --output /tmp/validation_test.json &> /dev/null

if [ -f "/tmp/validation_test.json" ]; then
    echo "  ✓ Test completed successfully"
    echo "  Results saved to: /tmp/validation_test.json"
    
    # Show some results
    if command -v jq &> /dev/null; then
        echo ""
        echo "  Quick Results:"
        jq '.results.throughput_ops_per_sec, .results.latency_ms.p50, .results.latency_ms.p99' /tmp/validation_test.json 2>/dev/null | while read line; do
            echo "    $line"
        done
    fi
else
    echo "  ✗ Test failed"
    exit 1
fi

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  ✓ Validation Complete - All Systems Operational"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "Next steps:"
echo "  1. Run full benchmarks: ./scripts/run_all.sh"
echo "  2. View results in: reports/"
echo "  3. See USAGE.md for detailed instructions"
echo ""

exit 0
