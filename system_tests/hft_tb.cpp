#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vstrategy.h"
#include <iostream>

int main(int argc, char** argv) {
    Verilated::commandArgs(argc, argv);
    Vstrategy* top = new Vstrategy;
    
    // Enable tracing for GTKWave
    Verilated::traceEverOn(true);
    VerilatedVcdC* tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("hft_waveform.vcd");

    // Sample market data (Price ticks)
    int prices[] = {1050, 1020, 995, 990, 1010};
    int ticks = sizeof(prices)/sizeof(prices[0]);

    for (int i = 0; i < 100; i++) {
        top->clk = !top->clk;
        if (i % 2 == 0) top->market_price = prices[i/2];
        
        top->eval();
        tfp->dump(i); // Save state for GTKWave
        
        if (top->buy_signal) {
            std::cout << "Order Placed at tick " << i/2 << " for Price: " << top->order_px << std::endl;
        }
    }

    tfp->close();
    delete top;
    return 0;
}
