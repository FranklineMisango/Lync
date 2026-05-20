#pragma once

#include <cstdint>
#include <vector>
#include <iostream>
#include <iomanip>

// Measures latency in simulation cycles and converts to nanoseconds
class SimulationLatencyMeasurer {
public:
    static constexpr double CLOCK_PERIOD_NS = 1.0;  // 1 GHz clock = 1ns per cycle

    struct CycleLatency {
        uint64_t start_cycle;
        uint64_t end_cycle;
        uint64_t cycle_count;
        
        double get_ns() const {
            return cycle_count * CLOCK_PERIOD_NS;
        }
    };

    void start_measurement(uint64_t current_cycle) {
        measurements.push_back({current_cycle, 0, 0});
    }

    void end_measurement(uint64_t current_cycle) {
        if (!measurements.empty() && measurements.back().end_cycle == 0) {
            measurements.back().end_cycle = current_cycle;
            measurements.back().cycle_count = current_cycle - measurements.back().start_cycle;
        }
    }

    void record_cycle_latency(uint64_t cycles) {
        cycle_latencies.push_back(cycles);
    }

    void print_cycle_stats(const std::string& label) const {
        if (measurements.empty()) {
            std::cout << "[" << label << "] No measurements recorded" << std::endl;
            return;
        }

        uint64_t min_cycles = measurements[0].cycle_count;
        uint64_t max_cycles = measurements[0].cycle_count;
        double total_cycles = 0;

        for (const auto& m : measurements) {
            if (m.cycle_count < min_cycles) min_cycles = m.cycle_count;
            if (m.cycle_count > max_cycles) max_cycles = m.cycle_count;
            total_cycles += m.cycle_count;
        }

        double avg_cycles = total_cycles / measurements.size();
        double avg_ns = avg_cycles * CLOCK_PERIOD_NS;

        std::cout << "\n========== " << label << " ==========" << std::endl;
        std::cout << "  Measurements: " << measurements.size() << std::endl;
        std::cout << "  Min Cycles:   " << min_cycles << " (" 
                  << std::fixed << std::setprecision(2) << min_cycles * CLOCK_PERIOD_NS << " ns)" << std::endl;
        std::cout << "  Max Cycles:   " << max_cycles << " (" 
                  << max_cycles * CLOCK_PERIOD_NS << " ns)" << std::endl;
        std::cout << "  Avg Cycles:   " << std::fixed << std::setprecision(1) << avg_cycles << " (" 
                  << avg_ns << " ns)" << std::endl;
        std::cout << "====================================\n" << std::endl;
    }

    double get_average_ns() const {
        if (measurements.empty()) return 0;
        double total_cycles = 0;
        for (const auto& m : measurements) {
            total_cycles += m.cycle_count;
        }
        return (total_cycles / measurements.size()) * CLOCK_PERIOD_NS;
    }

    void clear() {
        measurements.clear();
        cycle_latencies.clear();
    }

private:
    std::vector<CycleLatency> measurements;
    std::vector<uint64_t> cycle_latencies;
};

// Helper to convert between different clock speeds
class ClockConverter {
public:
    // Standard clock frequencies
    static constexpr double FREQ_1GHZ_NS = 1.0;    // 1 ns per cycle
    static constexpr double FREQ_500MHZ_NS = 2.0;  // 2 ns per cycle
    static constexpr double FREQ_100MHZ_NS = 10.0; // 10 ns per cycle

    static double cycles_to_ns(uint64_t cycles, double clock_period_ns) {
        return cycles * clock_period_ns;
    }

    static uint64_t ns_to_cycles(double ns, double clock_period_ns) {
        return (uint64_t)(ns / clock_period_ns);
    }

    // Print latency comparison for different clock speeds
    static void print_latency_comparison(uint64_t cycles) {
        std::cout << "\nLatency at different clock speeds:" << std::endl;
        std::cout << "  @ 1 GHz:   " << std::fixed << std::setprecision(2) 
                  << cycles_to_ns(cycles, FREQ_1GHZ_NS) << " ns" << std::endl;
        std::cout << "  @ 500 MHz: " << cycles_to_ns(cycles, FREQ_500MHZ_NS) << " ns" << std::endl;
        std::cout << "  @ 100 MHz: " << cycles_to_ns(cycles, FREQ_100MHZ_NS) << " ns" << std::endl;
    }
};
