module order_id_tracker (
    input  logic        clk,
    input  logic        reset,
    
    // Write Port (From Add Order Messages)
    input  logic        write_en,
    input  logic [63:0] write_order_id,
    input  logic        write_side,
    input  logic [31:0] write_price,
    input  logic [31:0] write_shares,
    
    // Read Port (From Cancel/Delete Messages)
    input  logic        read_en,
    input  logic [63:0] read_order_id,
    
    // Read Outputs (Available 1 clock cycle after read_en)
    output logic        read_valid,
    output logic        out_side,
    output logic [31:0] out_price,
    output logic [31:0] out_shares
);

    parameter int ADDR_WIDTH = 16;
    localparam int MEM_DEPTH = 1 << ADDR_WIDTH;

    // XOR folding hash for single-cycle index generation
    function logic [ADDR_WIDTH-1:0] get_hash(input logic [63:0] id);
        return id[ADDR_WIDTH-1:0] ^ id[2*ADDR_WIDTH-1:ADDR_WIDTH];
    endfunction

    logic [63:0] mem_order_id [MEM_DEPTH];
    logic        mem_side     [MEM_DEPTH];
    logic [31:0] mem_price    [MEM_DEPTH];
    logic [31:0] mem_shares   [MEM_DEPTH];
    logic        mem_allocated[MEM_DEPTH];

    logic [ADDR_WIDTH-1:0] write_idx;
    logic [ADDR_WIDTH-1:0] read_idx;

    assign write_idx = get_hash(write_order_id);
    assign read_idx  = get_hash(read_order_id);

    always_ff @(posedge clk) begin
        if (reset) begin
            read_valid <= 1'b0;
            out_side   <= 1'b0;
            out_price  <= 32'b0;
            out_shares <= 32'b0;
            for (int i = 0; i < MEM_DEPTH; i++) begin
                mem_allocated[i] = 1'b0;
            end
        end else begin
            if (write_en) begin
                mem_order_id[write_idx]  <= write_order_id;
                mem_side[write_idx]      <= write_side;
                mem_price[write_idx]     <= write_price;
                mem_shares[write_idx]    <= write_shares;
                mem_allocated[write_idx] <= 1'b1;
            end

            if (read_en) begin
                if (mem_allocated[read_idx] && (mem_order_id[read_idx] == read_order_id)) begin
                    read_valid <= 1'b1;
                    out_side   <= mem_side[read_idx];
                    out_price  <= mem_price[read_idx];
                    out_shares <= mem_shares[read_idx];
                end else begin
                    read_valid <= 1'b0;
                end
            end else begin
                read_valid <= 1'b0;
            end
        end
    end
endmodule
