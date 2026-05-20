module hft_alpha_core (
    input  logic        clk,
    input  logic        reset,
    
    // Parser Interface
    input  logic        msg_valid,
    input  logic [7:0]  msg_type,
    input  logic [15:0] msg_locate,
    input  logic        msg_side,
    input  logic [31:0] msg_price,
    input  logic [31:0] msg_shares,
    
    // Tracker Memory Interface (Interacting with the memory module)
    output logic        tracker_read_en,
    output logic [63:0] tracker_read_id,
    input  logic        tracker_read_valid,
    input  logic        tracker_out_side,
    input  logic [31:0] tracker_out_price,
    input  logic [31:0] tracker_out_shares,
    
    // Outbound Execution Core Interface
    output logic        send_order,
    output logic        order_side,
    output logic [31:0] order_price,
    output logic [31:0] order_qty
);

    parameter logic [15:0] TARGET_LOCATE   = 16'd1337; // E.g., AAPL Internal ID
    parameter logic [31:0] MAX_POSITION    = 32'd5000;
    parameter integer      IMBALANCE_ALPHA = 3;

    logic [63:0] total_bid_depth;
    logic [63:0] total_ask_depth;
    logic signed [31:0] current_position;
    logic               risk_vault_locked;

    // Control lookups for Cancel/Delete messages
    assign tracker_read_id = 64'h0; // Fed directly by Top-level logic for speed
    assign tracker_read_en = (msg_valid && (msg_locate == TARGET_LOCATE) && (msg_type == "D" || msg_type == "X"));

    always_ff @(posedge clk) begin
        if (reset) begin
            send_order         <= 1'b0;
            order_side         <= 1'b1;
            order_price        <= 32'b0;
            order_qty          <= 32'b0;
            total_bid_depth    <= 64'b0;
            total_ask_depth    <= 64'b0;
            current_position   <= 32'sd0;
            risk_vault_locked  <= 1'b0;
        end else begin
            send_order <= 1'b0; // Default pulse constraint

            if (msg_valid && (msg_locate == TARGET_LOCATE)) begin
                
                // Process Adds
                if (msg_type == "A" || msg_type == "F") begin
                    if (msg_side == 1'b1) total_bid_depth <= total_bid_depth + msg_shares;
                    else                  total_ask_depth <= total_ask_depth + msg_shares;
                end
            end

            // Process Cancels/Deletes based on memory tracker feedback (1 cycle latency delay)
            if (tracker_read_valid) begin
                if (tracker_out_side == 1'b1)
                    total_bid_depth <= (total_bid_depth > tracker_out_shares) ? (total_bid_depth - tracker_out_shares) : 64'b0;
                else
                    total_ask_depth <= (total_ask_depth > tracker_out_shares) ? (total_ask_depth - tracker_out_shares) : 64'b0;
            end

            // Signal Alpha Generation
            if (!risk_vault_locked) begin
                if ((total_bid_depth > (total_ask_depth * IMBALANCE_ALPHA)) && (total_ask_depth > 0)) begin
                    if ((current_position + 32'sd100) <= $signed(MAX_POSITION)) begin
                        send_order       <= 1'b1;
                        order_side       <= 1'b1; // BUY
                        order_price      <= msg_price + 32'd100; // Aggressive marketable cross
                        order_qty        <= 32'd100;
                        current_position <= current_position + 32'sd100;
                    end
                end
            end

            // Structural Hardware Boundary Check
            if (current_position > $signed(MAX_POSITION) || current_position < -$signed(MAX_POSITION)) begin
                risk_vault_locked <= 1'b1;
            end
        end
    end
endmodule
