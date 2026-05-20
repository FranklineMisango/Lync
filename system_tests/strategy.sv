module strategy (
    input logic clk,
    input logic reset,
    input logic [15:0] market_price,  // Incoming price from exchange
    output logic buy_signal,
    output logic [15:0] order_px
);
    parameter logic [15:0] BUY_THRESHOLD = 16'd1000;

    always_ff @(posedge clk) begin
        if (reset) begin
            buy_signal <= 1'b0;
            order_px   <= 16'b0;
        end else begin
            // Trigger buy if price is low
            if (market_price < BUY_THRESHOLD && market_price != 0) begin
                buy_signal <= 1'b1;
                order_px   <= market_price;
            end else begin
                buy_signal <= 1'b0;
            end
        end
    end
endmodule
