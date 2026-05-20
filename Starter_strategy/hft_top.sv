module hft_top (
    input  logic        clk,
    input  logic        reset,
    
    // ITCH 5.0 Input Interface
    input  logic        itch_msg_valid,
    input  logic [7:0]  itch_msg_type,
    input  logic [15:0] itch_msg_locate,
    input  logic        itch_msg_side,
    input  logic [31:0] itch_msg_price,
    input  logic [31:0] itch_msg_shares,
    input  logic [63:0] itch_msg_order_id,
    
    // OUCH Network Outbound Interface
    output logic        ouch_tvalid,
    output logic [64:0] ouch_tdata,
    output logic [7:0]  ouch_tkeep,
    output logic        ouch_tlast,
    input  logic        ouch_tready
);

    // Structural Interconnect Wires
    logic        t_read_en, t_read_valid, t_side;
    logic [63:0] t_read_id;
    logic [31:0] t_price, t_shares;
    
    logic        strat_send, strat_side;
    logic [31:0] strat_price, strat_qty;

    // 1. Order Book Storage Allocation Tracker
    order_id_tracker id_table (
        .clk(clk), .reset(reset),
        .write_en(itch_msg_valid && (itch_msg_type == "A" || itch_msg_type == "F")),
        .write_order_id(itch_msg_order_id),
        .write_side(itch_msg_side),
        .write_price(itch_msg_price),
        .write_shares(itch_msg_shares),
        .read_en(t_read_en),
        .read_order_id(itch_msg_order_id), // Shared input bus
        .read_valid(t_read_valid),
        .out_side(t_side),
        .out_price(t_price),
        .out_shares(t_shares)
    );

    // 2. Alpha Execution Kernel Strategy
    hft_alpha_core strategy_core (
        .clk(clk), .reset(reset),
        .msg_valid(itch_msg_valid),
        .msg_type(itch_msg_type),
        .msg_locate(itch_msg_locate),
        .msg_side(itch_msg_side),
        .msg_price(itch_msg_price),
        .msg_shares(itch_msg_shares),
        .tracker_read_en(t_read_en),
        .tracker_read_id(t_read_id),
        .tracker_read_valid(t_read_valid),
        .tracker_out_side(t_side),
        .tracker_out_price(t_price),
        .tracker_out_shares(t_shares),
        .send_order(strat_send),
        .order_side(strat_side),
        .order_price(strat_price),
        .order_qty(strat_qty)
    );

    // 3. Execution Network Frame Wrapper
    ouch_packet_generator ouch_gen (
        .clk(clk), .reset(reset),
        .trigger_trade(strat_send),
        .trade_side(strat_side),
        .trade_price(strat_price),
        .trade_qty(strat_qty),
        .tvalid(ouch_tvalid),
        .tdata(ouch_tdata),
        .tkeep(ouch_tkeep),
        .tlast(ouch_tlast),
        .tready(ouch_tready)
    );

endmodule
