#pragma once

#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>
#include <iomanip>

class LatencyTracker {
public:
    struct LatencyStats {
        double min_ns;
        double max_ns;
        double mean_ns;
        double median_ns;
        double p99_ns;
        double std_dev_ns;
        size_t sample_count;
    };

    void start_timing() {
        start_time = std::chrono::high_resolution_clock::now();
    }

    void end_timing() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end_time - start_time).count();
        latencies_ns.push_back(latency_ns);
    }

    void record_latency_ns(double latency_ns) {
        latencies_ns.push_back(latency_ns);
    }

    LatencyStats compute_stats() const {
        if (latencies_ns.empty()) {
            return {0, 0, 0, 0, 0, 0, 0};
        }

        auto sorted = latencies_ns;
        std::sort(sorted.begin(), sorted.end());

        double min_ns = sorted.front();
        double max_ns = sorted.back();
        double mean_ns = std::accumulate(sorted.begin(), sorted.end(), 0.0) / sorted.size();

        // Median
        double median_ns = 0;
        if (sorted.size() % 2 == 0) {
            median_ns = (sorted[sorted.size() / 2 - 1] + sorted[sorted.size() / 2]) / 2.0;
        } else {
            median_ns = sorted[sorted.size() / 2];
        }

        // P99
        size_t p99_idx = std::min(size_t(sorted.size() * 0.99), sorted.size() - 1);
        double p99_ns = sorted[p99_idx];

        // Standard deviation
        double variance = 0;
        for (double val : sorted) {
            variance += (val - mean_ns) * (val - mean_ns);
        }
        variance /= sorted.size();
        double std_dev_ns = std::sqrt(variance);

        return {min_ns, max_ns, mean_ns, median_ns, p99_ns, std_dev_ns, sorted.size()};
    }

    void print_stats(const std::string& label) const {
        auto stats = compute_stats();
        if (stats.sample_count == 0) {
            std::cout << "[" << label << "] No samples collected" << std::endl;
            return;
        }

        std::cout << "\n========== " << label << " ==========" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  Samples:    " << stats.sample_count << std::endl;
        std::cout << "  Min:        " << stats.min_ns << " ns" << std::endl;
        std::cout << "  Max:        " << stats.max_ns << " ns" << std::endl;
        std::cout << "  Mean:       " << stats.mean_ns << " ns" << std::endl;
        std::cout << "  Median:     " << stats.median_ns << " ns" << std::endl;
        std::cout << "  Std Dev:    " << stats.std_dev_ns << " ns" << std::endl;
        std::cout << "  P99:        " << stats.p99_ns << " ns" << std::endl;
        std::cout << "====================================\n" << std::endl;
    }

    void clear() {
        latencies_ns.clear();
    }

    size_t get_sample_count() const { return latencies_ns.size(); }
    const std::vector<double>& get_latencies() const { return latencies_ns; }

private:
    std::vector<double> latencies_ns;
    std::chrono::high_resolution_clock::time_point start_time;
};
