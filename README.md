# Lync
Lync is an open-source, FPGA-targeted trading engine designed for ultra-low latency execution. This project leverages Verilator for cycle-accurate C++ simulation and GTKWave for waveform analysis, providing a sandbox for developing hardware-accelerated trading strategies.

## Key Features

* Protocol Parsers: Hardware-level decoders for Nasdaq ITCH (Market Data) and OUCH (Order Entry).
* L2 Order Book: Real-time BRAM-based order book reconstruction with price-level tracking.
* Pre-Trade Risk Engine: Wire-speed validation of order size, price collars, and position limits (SEC 15c3-5 compliant).
* Verilator Testbench: A high-performance C++ simulation environment that feeds real PCAP market data into the HDL model.
* Latency Optimized: Designed for AXI-Stream pipelining to achieve deterministic, nanosecond-level processing.

## Tech Stack

* HDL: SystemVerilog / Verilog
* Simulation: Verilator (C++ Wrapper)
* Visualization: GTKWave (VCD/FST)
* Target Hardware: AMD/Xilinx UltraScale+ or Intel Stratix 10

