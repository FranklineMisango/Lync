module itch_stream_parser (
    input  logic       clk,
    input  logic       rst,
    input  logic       stream_valid,
    input  logic [7:0] stream_data,
    output logic       stream_ready,
    output logic       msg_valid,
    output logic [7:0] msg_type,
    output logic       msg_error,
    output logic       add_order_valid,
    output logic       stock_directory_valid,
    output logic       order_executed_valid,
    output logic       order_cancel_valid,
    output logic       order_delete_valid,
    output logic       order_replace_valid,
    output logic       trade_valid,
    output logic       cross_trade_valid,
    output logic       system_event_valid,
    output logic [31:0] total_messages,
    output logic [31:0] filtered_messages,
    output logic       symbol_match
);
    import itch_symbol_filter_pkg::*;

    typedef enum logic [1:0] {
        READ_LEN_HI,
        READ_LEN_LO,
        READ_PAYLOAD
    } state_t;

    state_t      state;
    logic [15:0] message_length;
    logic [15:0] payload_index;
    logic [7:0]  current_msg_type;
    logic [63:0] symbol_capture;

    assign stream_ready = 1'b1;

    function automatic logic [15:0] message_payload_length(input logic [7:0] type_char);
        case (type_char)
            8'h41: return 36;
            8'h45: return 31;
            8'h58: return 23;
            8'h53: return 12;
            8'h52: return 39;
            8'h44: return 19;
            8'h55: return 35;
            8'h50: return 44;
            8'h51: return 40;
            default: return 0;
        endcase
    endfunction

    function automatic bit has_symbol_field(input logic [7:0] type_char);
        case (type_char)
            8'h41,
            8'h52,
            8'h50,
            8'h51: return 1'b1;
            default: return 1'b0;
        endcase
    endfunction

    function automatic int symbol_offset(input logic [7:0] type_char);
        case (type_char)
            8'h41: return 24;
            8'h52: return 11;
            8'h50: return 24;
            8'h51: return 11;
            default: return -1;
        endcase
    endfunction

    function automatic logic [63:0] write_symbol_byte(
        input logic [63:0] current,
        input int          start_index,
        input int          index,
        input logic [7:0]  data
    );
        logic [63:0] next_value;

        next_value = current;
        case (index - start_index)
            0: next_value[63:56] = data;
            1: next_value[55:48] = data;
            2: next_value[47:40] = data;
            3: next_value[39:32] = data;
            4: next_value[31:24] = data;
            5: next_value[23:16] = data;
            6: next_value[15:8]  = data;
            7: next_value[7:0]   = data;
            default: ;
        endcase

        return next_value;
    endfunction

    always_ff @(posedge clk) begin
        if (rst) begin
            state                  <= READ_LEN_HI;
            message_length         <= 0;
            payload_index          <= 0;
            current_msg_type       <= 8'd0;
            symbol_capture         <= 64'd0;
            msg_valid              <= 1'b0;
            msg_type               <= 8'd0;
            msg_error              <= 1'b0;
            add_order_valid        <= 1'b0;
            stock_directory_valid  <= 1'b0;
            order_executed_valid   <= 1'b0;
            order_cancel_valid     <= 1'b0;
            order_delete_valid     <= 1'b0;
            order_replace_valid    <= 1'b0;
            trade_valid            <= 1'b0;
            cross_trade_valid      <= 1'b0;
            system_event_valid     <= 1'b0;
            total_messages         <= 32'd0;
            filtered_messages      <= 32'd0;
            symbol_match           <= 1'b0;
        end else begin
            msg_valid             <= 1'b0;
            msg_error             <= 1'b0;
            add_order_valid       <= 1'b0;
            stock_directory_valid <= 1'b0;
            order_executed_valid   <= 1'b0;
            order_cancel_valid     <= 1'b0;
            order_delete_valid     <= 1'b0;
            order_replace_valid    <= 1'b0;
            trade_valid            <= 1'b0;
            cross_trade_valid      <= 1'b0;
            system_event_valid     <= 1'b0;

            if (stream_valid) begin
                logic [7:0]        type_next;
                logic [63:0]       symbol_next;
                logic [15:0]       message_length_next;
                int                offset;
                bit                passes_filter;

                type_next           = current_msg_type;
                symbol_next         = symbol_capture;
                message_length_next = message_length;
                offset              = symbol_offset(type_next);

                case (state)
                    READ_LEN_HI: begin
                        message_length[15:8] <= stream_data;
                        state                <= READ_LEN_LO;
                    end

                    READ_LEN_LO: begin
                        message_length_next = {message_length[15:8], stream_data};
                        message_length      <= {message_length[15:8], stream_data};
                        payload_index       <= 0;
                        symbol_capture      <= 64'd0;
                        current_msg_type    <= 8'd0;
                        state               <= READ_PAYLOAD;
                    end

                    READ_PAYLOAD: begin
                        type_next = (payload_index == 0) ? stream_data : current_msg_type;

                        if (payload_index == 0) begin
                            current_msg_type <= stream_data;
                            offset           = symbol_offset(stream_data);
                        end

                        if (has_symbol_field(type_next) && offset >= 0 && int'(payload_index) >= offset && int'(payload_index) < (offset + 8)) begin
                            symbol_next = write_symbol_byte(symbol_next, offset, int'(payload_index), stream_data);
                        end

                        if ((payload_index + 1) == message_length_next) begin
                            total_messages <= total_messages + 1;
                            msg_type       <= type_next;

                            if (message_payload_length(type_next) == 0) begin
                                msg_error <= 1'b1;
                            end

                            if (has_symbol_field(type_next)) begin
                                passes_filter = is_symbol_filtered(symbol_next);
                                symbol_match  <= passes_filter;
                            end else begin
                                passes_filter = 1'b1;
                                symbol_match  <= 1'b1;
                            end

                            if (passes_filter) begin
                                filtered_messages <= filtered_messages + 1;
                                msg_valid         <= 1'b1;

                                case (type_next)
                                    8'h41: add_order_valid       <= 1'b1;
                                    8'h52: stock_directory_valid <= 1'b1;
                                    8'h45: order_executed_valid  <= 1'b1;
                                    8'h58: order_cancel_valid    <= 1'b1;
                                    8'h44: order_delete_valid    <= 1'b1;
                                    8'h55: order_replace_valid   <= 1'b1;
                                    8'h50: trade_valid           <= 1'b1;
                                    8'h51: cross_trade_valid     <= 1'b1;
                                    8'h53: system_event_valid    <= 1'b1;
                                    default: msg_error           <= 1'b1;
                                endcase
                            end

                            state         <= READ_LEN_HI;
                            payload_index <= 0;
                            symbol_capture <= 64'd0;
                        end else begin
                            payload_index  <= payload_index + 1;
                            symbol_capture <= symbol_next;
                        end
                    end

                    default: state <= READ_LEN_HI;
                endcase
            end
        end
    end
endmodule
