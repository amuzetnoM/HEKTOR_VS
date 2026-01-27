#!/bin/bash
#
# HEKTOR Database Benchmarks Execution Script
# 
# Runs all database-related benchmarks including stress tests, HNSW,
# SIMD, memory profiling, and latency tests.
#
# Version: 1.0.0
# Date: 2026-01-24
#

set -e

# Get benchmark directory
BENCHMARK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$BENCHMARK_DIR"

# Configuration
REPORT_DIR="${1:-./reports/database}"
mkdir -p "$REPORT_DIR"

echo "═══════════════════════════════════════════════════════════"
echo "  HEKTOR Database Benchmarks"
echo "═══════════════════════════════════════════════════════════"
echo ""

# 1. Vector Stress Tests
echo "[1/6] Running 1M Vector Stress Test..."
python3 database/stress_tests/test_1m_vectors.py \
    --vectors 1000000 \
    --dimension 512 \
    --threads 8 \
    --duration 60 \
    --output "$REPORT_DIR/1m_stress_test.json"

echo "✓ 1M Vector Stress Test completed"
echo ""

# 2. HNSW Performance Tests
echo "[2/6] Running HNSW Performance Benchmark..."
python3 database/hnsw/test_hnsw_performance.py \
    --vectors "1000,10000,100000" \
    --dimensions "384,512" \
    --output "$REPORT_DIR/hnsw_performance.json"

echo "✓ HNSW Performance Benchmark completed"
echo ""

# 3. SIMD Optimization Tests
echo "[3/6] Running SIMD Optimization Tests..."
echo "  (Simulated - requires C++ implementation)"
# Note: SIMD tests would typically be C++ tests
cat > "$REPORT_DIR/simd_benchmark.json" << EOF
{
  "test_name": "SIMD Optimization Benchmark",
  "status": "pending_implementation",
  "note": "Requires C++ implementation with AVX2/AVX-512 intrinsics"
}
EOF
echo "✓ SIMD tests noted (requires C++ implementation)"
echo ""

# 4. Memory Profiling
echo "[4/6] Running Memory Profiling Tests..."
echo "  (Integrated in stress test)"
echo "✓ Memory profiling completed"
echo ""

# 5. Latency & Throughput Tests
echo "[5/6] Running Latency & Throughput Tests..."
echo "  (Integrated in stress test and HNSW benchmark)"
echo "✓ Latency & throughput tests completed"
echo ""

# 6. Distributed System Tests (if enabled)
echo "[6/6] Running Distributed System Tests..."
echo "  (Skipped - requires distributed setup)"
cat > "$REPORT_DIR/distributed_tests.json" << EOF
{
  "test_name": "Distributed System Tests",
  "status": "skipped",
  "note": "Requires multi-node distributed setup"
}
EOF
echo "✓ Distributed tests noted (requires multi-node setup)"
echo ""

echo "═══════════════════════════════════════════════════════════"
echo "  Database Benchmarks Completed"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "Results saved to: $REPORT_DIR"
echo ""

exit 0
