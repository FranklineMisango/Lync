module ouch_packet_generator (
    input  logic        clk,
    input  logic        reset,
    
    input  logic        trigger_trade,
    input  logic        trade_side,
    input  logic [31:0] trade_price,
    input  logic [31:0] trade_qty,
    
    output logic        tvalid,
    output logic [64:0] tdata,
    output logic [7:0]  tkeep,
    output logic        tlast,
    input  logic        tready
);

    typedef enum logic [2:0] {IDLE, HDR1, HDR2, OUCH1, OUCH2, OUCH3, DONE} state_t;
    state_t state;
    
    logic [31:0] r_price, r_qty;
    logic        r_side;
    logic [31:0] token;

    always_ff @(posedge clk) begin
        if (reset) begin
            state   <= IDLE;
            tvalid  <= 1'b0;
            tdata   <= 64'b0;
            tkeep   <= 8'b0;
            tlast   <= 1'b0;
            token   <= 32'd5000;
        end else begin
            case (state)
                IDLE: begin
                    tvalid <= 1'b0; tlast <= 1'b0; tkeep <= 8'b0;
                    if (trigger_trade && tready) begin
                        r_price <= trade_price;
                        r_qty   <= trade_qty;
                        r_side  <= trade_side;
                        token   <= token + 1'b1;
                        state   <= HDR1;
                    end
                end
                HDR1: begin
                    tvalid <= 1'b1; tkeep <= 8'hFF; tdata <= 64'h000F531420A1_FFFF;
                    state  <= HDR2;
                end
                HDR2: begin
                    tkeep <= 8'hFF; tdata <= 64'h4500_003C_0000_4000;
                    state <= OUCH1;
                end
                OUCH1: begin
                    tkeep <= 8'hFF; tdata <= {8'h4F, token, 24'h4D5946}; // 'O' + Token + FIRM prefix
                    state <= OUCH2;
                end
                OUCH2: begin
                    tkeep <= 8'hFF; tdata <= {8'h4D, (r_side ? 8'h42 : 8'h53), r_qty, 16'h2020};
                    state <= OUCH3;
                end
                OUCH3: begin
                    tkeep <= 8'hFF; tlast <= 1'b1; tdata <= {r_price, 32'h44415920}; // Price + "DAY "
                    state <= DONE;
                end
                DONE: begin
                    tvalid <= 1'b0; tlast <= 1'b0; tkeep <= 8'b0;
                    state  <= IDLE;
                end
            endcase
        end
    end
endmodule
