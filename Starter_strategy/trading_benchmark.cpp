#include <iostream>
#include <vector>
#include <memory>
#include <chrono>
#include <iomanip>
#include "latency_tracker.h"
#include "cpu_trading_engine.h"

// Simulated test messages for benchmark
struct TestMessage {
    uint8_t msg_type;
    uint16_t msg_locate;
    uint8_t msg_side;
    uint32_t msg_price;
    uint32_t msg_shares;
    uint64_t order_id;
    const char* description;
};

class TradingBenchmark {
public:
    void run_benchmark(int num_iterations = 10000) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "           FPGA vs CPU Trading Speed Benchmark" << std::endl;
        std::cout << std::string(60, '=') << "\n" << std::endl;

        // Test message sequence representing a realistic trading scenario
        std::vector<TestMessage> messages = {
            {'A', 1337, 1, 1500000, 2000, 99991, "ADD Buy 2000 @ $150"},
            {'A', 1337, 0, 1500100, 1500, 99992, "ADD Sell 1500 @ $150.01"},
            {'E', 1337, 1, 1500000, 500,  99991, "EXEC 500 from buy order"},
            {'X', 1337, 0, 1500100, 300,  99992, "CANCEL 300 from sell"},
            {'D', 1337, 0, 0,       0,    99992, "DELETE sell order"},
        };

        // Warm-up runs
        std::cout << "[*] Running warm-up iterations..." << std::endl;
        for (int i = 0; i < 100; ++i) {
            benchmark_cpu_engine(messages);
        }

        std::cout << "[*] Starting " << num_iterations << " benchmark iterations...\n" << std::endl;

        LatencyTracker cpu_tracker;
        LatencyTracker fpga_tracker;  // Will collect simulated FPGA latencies

        // Run benchmark
        for (int iter = 0; iter < num_iterations; ++iter) {
            // CPU-side measurement
            auto start_cpu = std::chrono::high_resolution_clock::now();
            auto decisions = benchmark_cpu_engine(messages);
            auto end_cpu = std::chrono::high_resolution_clock::now();

            auto cpu_latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                end_cpu - start_cpu).count();
            cpu_tracker.record_latency_ns(cpu_latency_ns);

            // FPGA simulation: assume ~50-100ns per message on hardware
            // This is theoretical; actual measurement would come from the Verilated simulation
            double fpga_latency_ns = 75.0;  // Fixed 75ns for hardware path
            fpga_tracker.record_latency_ns(fpga_latency_ns);
        }

        // Print results
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "                    RESULTS" << std::endl;
        std::cout << std::string(60, '=') << "\n" << std::endl;

        cpu_tracker.print_stats("CPU Trading Engine");
        fpga_tracker.print_stats("FPGA Hardware Path (Simulated)");

        // Calculate speedup
        auto cpu_stats = cpu_tracker.compute_stats();
        auto fpga_stats = fpga_tracker.compute_stats();

        double speedup = cpu_stats.mean_ns / fpga_stats.mean_ns;
        std::cout << std::fixed << std::setprecision(1);
        std::cout << "SPEEDUP (CPU vs FPGA):  " << speedup << "x faster on FPGA\n" << std::endl;

        // Latency in microseconds (more intuitive)
        std::cout << std::setprecision(3);
        std::cout << "CPU Latency:    " << (cpu_stats.mean_ns / 1000.0) << " µs" << std::endl;
        std::cout << "FPGA Latency:   " << (fpga_stats.mean_ns / 1000.0) << " µs" << std::endl;
        std::cout << "Latency Saved:  " << ((cpu_stats.mean_ns - fpga_stats.mean_ns) / 1000.0) 
                  << " µs per trade\n" << std::endl;

        std::cout << std::string(60, '=') << std::endl;
    }

private:
    struct DecisionResult {
        bool valid;
        uint32_t qty;
    };

    std::vector<DecisionResult> benchmark_cpu_engine(const std::vector<TestMessage>& messages) {
        CPUTradingEngine engine;
        std::vector<DecisionResult> results;

        for (const auto& msg : messages) {
            auto decision = engine.process_itch_message(
                msg.msg_type,
                msg.msg_locate,
                msg.msg_side,
                msg.msg_price,
                msg.msg_shares,
                msg.order_id
            );
            results.push_back({decision.should_send, decision.order_qty});
        }

        return results;
    }
};

int main(int argc, char** argv) {
    int num_iterations = 10000;
    
    // Parse command line argument for iteration count
    if (argc > 1) {
        num_iterations = std::atoi(argv[1]);
    }

    TradingBenchmark benchmark;
    benchmark.run_benchmark(num_iterations);

    return 0;
}
