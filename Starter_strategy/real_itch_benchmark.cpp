#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <chrono>
#include <iomanip>
#include <cstring>
#include "latency_tracker.h"
#include "cpu_trading_engine.h"

struct ITCHMessage {
    uint16_t length;
    uint8_t type;
    std::vector<uint8_t> payload;
};

class ITCHFileReader {
public:
    static std::vector<ITCHMessage> read_file(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open ITCH file: " + filename);
        }

        std::vector<ITCHMessage> messages;
        uint16_t length;

        while (file.read(reinterpret_cast<char*>(&length), 2)) {
            // Read in network byte order (big-endian)
            length = ((length & 0xFF) << 8) | ((length >> 8) & 0xFF);
            
            if (length == 0 || length > 10000) {
                break;  // Invalid message, stop reading
            }

            std::vector<uint8_t> buffer(length);
            if (!file.read(reinterpret_cast<char*>(buffer.data()), length)) {
                break;
            }

            ITCHMessage msg;
            msg.length = length;
            msg.type = buffer[0];
            msg.payload.insert(msg.payload.end(), buffer.begin() + 1, buffer.end());
            messages.push_back(msg);
        }

        file.close();
        return messages;
    }

    static std::string message_type_name(uint8_t type) {
        switch (type) {
            case 'S': return "System Event";
            case 'R': return "Stock Directory";
            case 'H': return "Stock Trading Action";
            case 'Y': return "Reg SHO Short Sale";
            case 'L': return "LULD Auction Collar";
            case 'V': return "MWCB Decline";
            case 'W': return "MWCB Status";
            case 'K': return "Limit Up-Down";
            case 'J': return "IPO Quoting";
            case 'A': return "Add Order";
            case 'F': return "Add Order MPID";
            case 'E': return "Execute Order";
            case 'C': return "Execute Order w/ Price";
            case 'X': return "Cancel Order";
            case 'D': return "Delete Order";
            case 'U': return "Replace Order";
            case 'P': return "Trade";
            case 'Q': return "Cross Trade";
            case 'B': return "Broken Trade";
            case 'I': return "NOII";
            case 'N': return "RPII";
            case 'Z': return "Timestamp";
            default: return "Unknown";
        }
    }
};

class RealITCHBenchmark {
public:
    void run_benchmark(const std::string& itch_file, int max_messages = 10000) {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "  Real ITCH Data Benchmark" << std::endl;
        std::cout << "  File: " << itch_file << std::endl;
        std::cout << std::string(70, '=') << "\n" << std::endl;

        // Load ITCH data
        std::cout << "[*] Loading ITCH file..." << std::endl;
        auto messages = ITCHFileReader::read_file(itch_file);
        std::cout << "[✓] Loaded " << messages.size() << " messages\n" << std::endl;

        if (messages.empty()) {
            std::cerr << "[ERROR] No messages found in ITCH file" << std::endl;
            return;
        }

        // Analyze message composition
        std::map<uint8_t, int> msg_type_counts;
        for (const auto& msg : messages) {
            msg_type_counts[msg.type]++;
        }

        std::cout << "Message Type Distribution:" << std::endl;
        for (const auto& [type, count] : msg_type_counts) {
            std::cout << "  " << ITCHFileReader::message_type_name(type) 
                      << " (" << static_cast<char>(type) << "): " << count << " msgs" << std::endl;
        }
        std::cout << std::endl;

        // Setup latency trackers
        LatencyTracker cpu_tracker;
        LatencyTracker msg_processing_time;
        
        int process_count = std::min(max_messages, (int)messages.size());
        std::cout << "Processing first " << process_count << " messages...\n" << std::endl;

        CPUTradingEngine engine;

        // Process each message
        for (int i = 0; i < process_count; ++i) {
            const auto& msg = messages[i];

            // Time the CPU processing
            auto start = std::chrono::high_resolution_clock::now();

            // Parse and process based on message type
            process_message(engine, msg);

            auto end = std::chrono::high_resolution_clock::now();
            double latency_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                end - start).count();

            cpu_tracker.record_latency_ns(latency_ns);
            msg_processing_time.record_latency_ns(latency_ns);

            // Print first 10 messages
            if (i < 10) {
                std::cout << "[Msg " << i << "] Type: " 
                         << ITCHFileReader::message_type_name(msg.type) 
                         << " | Length: " << msg.length << " bytes | CPU: "
                         << std::fixed << std::setprecision(3) << latency_ns << " ns" << std::endl;
            }
        }

        // Print results
        print_results(cpu_tracker, process_count);
    }

private:
    void process_message(CPUTradingEngine& engine, const ITCHMessage& msg) {
        // Extract fields based on ITCH message type
        // This is simplified - full ITCH parsing would extract all fields

        uint8_t msg_type = msg.type;
        uint16_t locate_code = 1337;  // Default
        uint8_t side = 0;
        uint32_t price = 0;
        uint32_t shares = 0;
        uint64_t order_id = 0;

        // Parse relevant fields from payload based on message type
        if (msg.payload.size() >= 4) {
            // Most messages have locate code in first 2 bytes
            locate_code = (msg.payload[0] << 8) | msg.payload[1];
        }

        if (msg_type == 'A' || msg_type == 'F') {  // Add Order
            if (msg.payload.size() >= 35) {
                order_id = extract_uint64(&msg.payload[0]);  // Order ID (8 bytes at offset 4)
                side = msg.payload[13];  // Buy/Sell indicator
                shares = extract_uint32(&msg.payload[14]);   // Shares
                price = extract_uint32(&msg.payload[18]);    // Price
            }
        } else if (msg_type == 'E' || msg_type == 'C') {  // Execute
            if (msg.payload.size() >= 20) {
                order_id = extract_uint64(&msg.payload[0]);
                shares = extract_uint32(&msg.payload[8]);    // Executed shares
                price = extract_uint32(&msg.payload[12]);    // Execution price
            }
        } else if (msg_type == 'X') {  // Cancel
            if (msg.payload.size() >= 20) {
                order_id = extract_uint64(&msg.payload[0]);
                shares = extract_uint32(&msg.payload[8]);    // Canceled shares
            }
        } else if (msg_type == 'D') {  // Delete
            if (msg.payload.size() >= 8) {
                order_id = extract_uint64(&msg.payload[0]);
            }
        }

        // Process through CPU trading engine
        engine.process_itch_message(msg_type, locate_code, side, price, shares, order_id);
    }

    uint64_t extract_uint64(const uint8_t* ptr) const {
        uint64_t val = 0;
        for (int i = 0; i < 8; ++i) {
            val = (val << 8) | ptr[i];
        }
        return val;
    }

    uint32_t extract_uint32(const uint8_t* ptr) const {
        uint32_t val = 0;
        for (int i = 0; i < 4; ++i) {
            val = (val << 8) | ptr[i];
        }
        return val;
    }

    void print_results(const LatencyTracker& tracker, int msg_count) {
        std::cout << "\n" << std::string(70, '=') << std::endl;
        std::cout << "                    BENCHMARK RESULTS" << std::endl;
        std::cout << std::string(70, '=') << "\n" << std::endl;

        tracker.print_stats("CPU Processing (Real ITCH Data)");

        auto stats = tracker.compute_stats();

        std::cout << std::fixed << std::setprecision(3);
        std::cout << "Total Messages:        " << msg_count << std::endl;
        std::cout << "Total Time:            " << (stats.mean_ns * msg_count / 1e6) << " ms" << std::endl;
        std::cout << "Throughput:            " << (msg_count * 1e9 / (stats.mean_ns * msg_count)) 
                  << " msgs/sec" << std::endl;

        // Compare with theoretical FPGA (75ns per message)
        double fpga_time_per_msg = 75.0;  // ns
        double speedup = stats.mean_ns / fpga_time_per_msg;
        std::cout << "\nFPGA Speedup (Theoretical @ 75ns):" << std::endl;
        std::cout << "  Speedup Factor:  " << std::setprecision(1) << speedup << "x" << std::endl;
        std::cout << "  Time Saved:      " << std::setprecision(3) 
                  << (stats.mean_ns - fpga_time_per_msg) / 1000.0 << " µs per message" << std::endl;
        std::cout << "  Total Saved:     " 
                  << ((stats.mean_ns - fpga_time_per_msg) * msg_count / 1e6) << " ms for batch" << std::endl;

        std::cout << "\n" << std::string(70, '=') << std::endl;
    }
};

int main(int argc, char** argv) {
    std::string itch_file = "data/real_sample_slice_wide.itch";
    int max_messages = 10000;

    if (argc > 1) {
        itch_file = argv[1];
    }
    if (argc > 2) {
        max_messages = std::atoi(argv[2]);
    }

    try {
        RealITCHBenchmark benchmark;
        benchmark.run_benchmark(itch_file, max_messages);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
