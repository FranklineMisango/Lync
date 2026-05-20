#!/bin/bash

# Verilator Build Script for HFT Top Module with Latency Benchmarking
# Compiles hft_top.sv and links with latency measurement testbench

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DIR="$PROJECT_DIR/Starter_strategy"
OBJ_DIR="$PROJECT_DIR/obj_dir"
BUILD_DIR="$PROJECT_DIR/build"

mkdir -p "$BUILD_DIR"

echo "================================================"
echo "  Building FPGA HFT Top Module with Verilator"
echo "================================================"
echo ""

# Step 1: Verilate the design
echo "[1/3] Verilating hft_top.sv..."
verilator \
    --cc \
    --trace \
    --exe \
    "$SRC_DIR/hft_top.sv" \
    "$SRC_DIR/itch_stream_parser.sv" \
    "$SRC_DIR/alpha_core.sv" \
    "$SRC_DIR/order_id_tracker.sv" \
    "$SRC_DIR/ouch_parket_generator.sv" \
    "$SRC_DIR/itch_symbol_filter_pkg.sv" \
    -I"$SRC_DIR" \
    -CFLAGS "-O3 -march=native" \
    --output-split 20000 \
    --output-split-cfuncs 5000 \
    -o hft_fpga_benchmark

echo "      ✓ Verilation complete"
echo ""

# Step 2: Prepare benchmark source
echo "[2/3] Preparing benchmark harness..."
cat > "$OBJ_DIR/hft_fpga_benchmark_main.cpp" << 'EOF'
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <cstring>
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vhft_top.h"
#include "../Starter_strategy/latency_tracker.h"

vluint64_t main_time = 0;

double sc_time_stamp() {
    return main_time;
}

struct MessageLatency {
    uint64_t arrival_cycle;
    uint64_t send_cycle;
    uint64_t latency_cycles;
    double latency_ns;
};

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Verilated::traceEverOn(true);
    
    auto top = std::make_unique<Vhft_top>();
    LatencyTracker fpga_latencies;
    std::vector<MessageLatency> measurements;
    
    std::cout << "\n================================================" << std::endl;
    std::cout << "  FPGA HFT Top Module - Real Latency Benchmark" << std::endl;
    std::cout << "================================================\n" << std::endl;
    
    // Reset sequence
    std::cout << "[*] Performing hardware reset..." << std::endl;
    top->clk = 0;
    top->reset = 1;
    top->itch_msg_valid = 0;
    top->ouch_tready = 1;
    
    for (int i = 0; i < 10; ++i) {
        top->clk = 0; top->eval();
        main_time++;
        top->clk = 1; top->eval();
        main_time++;
    }
    top->reset = 0;
    std::cout << "[✓] Hardware reset complete\n" << std::endl;
    
    // Test sequence: Inject representative ITCH messages
    std::cout << "[*] Running message latency measurements..." << std::endl;
    
    struct TestMessage {
        uint8_t type;
        uint16_t locate;
        uint8_t side;
        uint32_t price;
        uint32_t shares;
        uint64_t order_id;
        const char* desc;
    };
    
    std::vector<TestMessage> test_msgs = {
        {'A', 1337, 1, 1500000, 2000, 99991, "ADD Buy 2000 @ $150"},
        {'A', 1337, 0, 1500100, 1500, 99992, "ADD Sell 1500 @ $150.01"},
        {'E', 1337, 1, 1500000, 500,  99991, "EXEC 500"},
        {'X', 1337, 0, 1500100, 300,  99992, "CANCEL 300"},
    };
    
    for (const auto& msg : test_msgs) {
        // Clock cycle where message arrives
        uint64_t arrival_cycle = main_time;
        
        // Inject message
        top->itch_msg_valid = 1;
        top->itch_msg_type = msg.type;
        top->itch_msg_locate = msg.locate;
        top->itch_msg_side = msg.side;
        top->itch_msg_price = msg.price;
        top->itch_msg_shares = msg.shares;
        top->itch_msg_order_id = msg.order_id;
        
        // Clock it in
        top->clk = 0; top->eval();
        main_time++;
        top->clk = 1; top->eval();
        main_time++;
        
        // Release message
        top->itch_msg_valid = 0;
        
        // Monitor for order transmission (poll output for ~50 cycles max)
        uint64_t send_cycle = 0;
        for (int cycle = 0; cycle < 100; ++cycle) {
            top->clk = 0; top->eval();
            main_time++;
            top->clk = 1; top->eval();
            main_time++;
            
            // Check if order was sent (ouch_tvalid)
            if (top->ouch_tvalid) {
                send_cycle = main_time;
                top->ouch_tvalid = 0; // Clear for next message
                break;
            }
        }
        
        // Record latency
        uint64_t latency_cycles = send_cycle - arrival_cycle;
        double latency_ns = latency_cycles * 1.0;  // 1 GHz clock
        
        MessageLatency meas;
        meas.arrival_cycle = arrival_cycle;
        meas.send_cycle = send_cycle;
        meas.latency_cycles = latency_cycles;
        meas.latency_ns = latency_ns;
        measurements.push_back(meas);
        
        std::cout << "  [" << msg.desc << "] " 
                 << latency_cycles << " cycles = "
                 << std::fixed << std::setprecision(1) << latency_ns << " ns" << std::endl;
        
        fpga_latencies.record_latency_ns(latency_ns);
    }
    
    // Print results
    std::cout << "\n" << std::string(50, '=') << std::endl;
    fpga_latencies.print_stats("FPGA Hardware Latency");
    
    auto stats = fpga_latencies.compute_stats();
    std::cout << "\nKey Metrics:" << std::endl;
    std::cout << "  Min Cycles:  " << (uint64_t)stats.min_ns << " cycles" << std::endl;
    std::cout << "  Max Cycles:  " << (uint64_t)stats.max_ns << " cycles" << std::endl;
    std::cout << "  Mean Cycles: " << (uint64_t)stats.mean_ns << " cycles" << std::endl;
    
    return 0;
}
EOF

# Step 3: Build
echo "      ✓ Benchmark harness prepared"
echo ""
echo "[3/3] Compiling with Verilated objects..."
cd "$OBJ_DIR"

# Add the benchmark source to the make command
make -f Vhft_top.mk CXXFLAGS="-O3 -march=native" 2>&1 | tail -20

if [ -f "hft_fpga_benchmark" ]; then
    echo "      ✓ Build successful"
else
    echo "      ✗ Build failed"
    exit 1
fi

echo ""
echo "================================================"
echo "  ✓ Build Complete!"
echo "================================================"
echo ""
echo "Run the FPGA benchmark:"
echo "  cd $OBJ_DIR && ./hft_fpga_benchmark"
echo ""
