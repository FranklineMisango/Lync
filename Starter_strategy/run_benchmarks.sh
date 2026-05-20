#!/bin/bash

# HFT Benchmark Suite Runner
# Automatically compiles and runs all benchmarks, collecting results

set -e

BENCHMARK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="$BENCHMARK_DIR/benchmark_results"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_FILE="$RESULTS_DIR/benchmark_${TIMESTAMP}.txt"

# Create results directory
mkdir -p "$RESULTS_DIR"

echo "=================================================="
echo "   HFT Trading Speed Benchmark Suite v1.0"
echo "   $(date)"
echo "=================================================="
echo ""

# Check if we're in the right directory
if [ ! -f "Starter_strategy/trading_benchmark.cpp" ]; then
    echo "[ERROR] Must run from /home/misango/codechest/Lync directory"
    exit 1
fi

# Compile benchmarks
echo "[1/3] Compiling CPU benchmark..."
cd Starter_strategy
g++ -O3 -std=c++17 -march=native -ffast-math trading_benchmark.cpp -o trading_benchmark
echo "      ✓ Compiled successfully"
echo ""

# Run CPU benchmark
echo "[2/3] Running CPU benchmark (10,000 iterations)..."
echo ""
./trading_benchmark 10000 | tee "$RESULT_FILE"
echo ""

# Check for real data
if [ -f "../data/real_sample_slice.itch" ]; then
    echo "[3/3] Real data benchmark available, but requires Verilator build"
    echo "      To run with real ITCH data:"
    echo "      1. cd .. && make -f Makefile"
    echo "      2. ./obj_dir/Vhft_top data/real_sample_slice.itch 1000"
else
    echo "[3/3] Real data not found at ../data/real_sample_slice.itch"
fi

echo ""
echo "=================================================="
echo "Benchmark Results Summary"
echo "=================================================="
echo "Results saved to: $RESULT_FILE"
echo ""

# Extract key metrics
echo "Key Findings:"
grep -E "SPEEDUP|Latency|P99" "$RESULT_FILE" || true

echo ""
echo "For detailed results, run:"
echo "  cat $RESULT_FILE"
echo ""
echo "To compare with previous runs:"
echo "  diff $RESULTS_DIR/benchmark_*.txt"
