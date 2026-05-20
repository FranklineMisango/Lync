#include <verilated.h>

#include "Vitch_stream_parser.h"

#include <fstream>
#include <map>
#include <iostream>
#include <iterator>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::vector<unsigned char> read_binary_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open input file: " + path);
    }

    return std::vector<unsigned char>(std::istreambuf_iterator<char>(file), {});
}

bool ends_with(const std::string& value, const std::string& suffix) {
    if (value.size() < suffix.size()) {
        return false;
    }
    return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string symbol_to_string(std::uint64_t symbol) {
    std::string text(8, '\0');
    for (int i = 0; i < 8; ++i) {
        text[i] = static_cast<char>((symbol >> ((7 - i) * 8)) & 0xFF);
    }

    while (!text.empty() && (text.back() == ' ' || text.back() == '\0')) {
        text.pop_back();
    }

    return text;
}

char message_type_to_char(std::uint8_t value) {
    return static_cast<char>(value);
}

}  // namespace

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);

    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <raw_itch_file> [ticker|ALL]\n";
        std::cerr << "Hint: decompress Nasdaq .gz samples first, then pass the raw binary file.\n";
        return 1;
    }

    std::string input_path = argv[1];
    std::string target_ticker = (argc == 3) ? argv[2] : "ALL";
    bool report_all_tickers = (target_ticker == "ALL" || target_ticker == "*");
    if (ends_with(input_path, ".gz")) {
        std::cerr << "Error: expected a decompressed raw ITCH file, not a .gz archive.\n";
        return 1;
    }

    std::vector<unsigned char> data;
    try {
        data = read_binary_file(input_path);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    Vitch_stream_parser* top = new Vitch_stream_parser;

    top->clk = 0;
    top->rst = 1;
    top->stream_valid = 0;
    top->stream_data = 0;

    auto tick = [&](std::uint64_t cycle) {
        top->clk = 0;
        top->eval();

        top->clk = 1;
        top->eval();
    };

    std::uint64_t cycle = 0;
    for (int i = 0; i < 4; ++i) {
        tick(cycle++);
    }

    top->rst = 0;
    for (int i = 0; i < 2; ++i) {
        tick(cycle++);
    }

    std::uint64_t accepted_messages = 0;
    std::uint64_t matched_ticker_messages = 0;
    std::map<std::string, std::uint64_t> ticker_hits;

    for (unsigned char byte : data) {
        top->stream_valid = 1;
        top->stream_data = byte;
        tick(cycle++);

        if (top->msg_valid) {
            ++accepted_messages;
            char type_char = message_type_to_char(top->msg_type);
            std::string symbol = symbol_to_string(top->stock_symbol);

            if (report_all_tickers) {
                if (!symbol.empty()) {
                    ++ticker_hits[symbol];
                }
            } else if (symbol == target_ticker) {
                ++matched_ticker_messages;
                std::cout << "TICKER HIT: " << target_ticker
                          << " type='" << type_char << "'"
                          << " total=" << top->total_messages
                          << " filtered=" << top->filtered_messages
                          << " symbol_match=" << static_cast<int>(top->symbol_match)
                          << '\n';
            }

            (void)type_char;
            (void)symbol;
        }

        if (top->msg_error) {
            std::cerr << "Parser error at cycle " << cycle << '\n';
            break;
        }

        top->stream_valid = 0;
    }

    std::cout << "\nSimulation summary\n";
    std::cout << "  Raw bytes processed: " << data.size() << '\n';
    std::cout << "  Accepted messages:   " << accepted_messages << '\n';
    if (report_all_tickers) {
        std::uint64_t total_ticker_messages = 0;
        for (const auto& entry : ticker_hits) {
            total_ticker_messages += entry.second;
        }

        std::cout << "  Unique tickers:      " << ticker_hits.size() << '\n';
        std::cout << "  Ticker messages:     " << total_ticker_messages << '\n';
        std::cout << "\nPer-ticker summary\n";
        for (const auto& entry : ticker_hits) {
            std::cout << "  " << entry.first << ": " << entry.second << '\n';
        }
    } else {
        std::cout << "  Ticker hits (" << target_ticker << "): " << matched_ticker_messages << '\n';
    }
    std::cout << "  Parser total count:  " << top->total_messages << '\n';
    std::cout << "  Filtered count:      " << top->filtered_messages << '\n';

    delete top;
    return 0;
}
