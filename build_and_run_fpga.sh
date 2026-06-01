#!/bin/bash

# Build and run the FPGA simulation with latency benchmarking and FST waveform dumping
# This script builds hft_top with Verilator and links with sim_main.cpp for measurement
# After simulation, view signals with: gtkwave obj_dir_hft/hft_strategy.fst

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
SRC_DIR="$PROJECT_ROOT/Starter_strategy"
OBJ_DIR="$PROJECT_ROOT/obj_dir_hft"
BUILD_DIR="$PROJECT_ROOT/build_fpga"

mkdir -p "$OBJ_DIR" "$BUILD_DIR"

echo ""
echo "=================================================="
echo "  Building FPGA HFT Top with Latency Benchmarking"
echo "=================================================="
echo ""

cd "$OBJ_DIR"

# Step 1: Verilate the design
echo "[1/3] Verilating hft_top.sv and dependencies..."
verilator -Wno-fatal \
    --cc \
    --trace \
    --trace-fst \
    --exe \
    --build \
    -CFLAGS "-O3 -march=native" \
    -LDFLAGS "-lz" \
    -j $(nproc) \
    -o hft_fpga_benchmark \
    "$SRC_DIR/hft_top.sv" \
    "$SRC_DIR/itch_stream_parser.sv" \
    "$SRC_DIR/alpha_core.sv" \
    "$SRC_DIR/order_id_tracker.sv" \
    "$SRC_DIR/ouch_parket_generator.sv" \
    -I"$SRC_DIR" \
    "$SRC_DIR/sim_main.cpp"

if [ $? -eq 0 ]; then
    echo "      ✓ Verilator build successful"
else
    echo "      ✗ Verilator build failed"
    exit 1
fi

echo ""
echo "=================================================="
echo "  Build Complete!"
echo "=================================================="
echo ""
echo "Executable location: $OBJ_DIR/hft_fpga_benchmark"
echo ""
echo "Run the FPGA latency benchmark:"
echo "  cd $OBJ_DIR && ./hft_fpga_benchmark"
echo ""
echo "After running, view signals in GTKWave:"
echo "  gtkwave $OBJ_DIR/hft_strategy.fst"
echo ""

# Automatically run it
echo "Running FPGA Benchmark..."
echo "=================================================="
echo ""
cd "$OBJ_DIR"
"./hft_fpga_benchmark"
echo ""
echo "=================================================="
echo "  Simulation Complete!"
echo "=================================================="
echo ""
echo "Waveform saved to: $OBJ_DIR/hft_strategy.fst"
echo "View with: gtkwave $OBJ_DIR/hft_strategy.fst"
echo ""
