package itch_symbol_filter_pkg;
    typedef logic [63:0] symbol_t;
    typedef symbol_t symbol_array_t [0:7];

    localparam bit ENABLE_SYMBOL_FILTER = 1'b0;

    localparam symbol_array_t FILTER_SYMBOL_LIST = '{
        64'h4141504C20202020,  // AAPL    
        64'h54534C4120202020,  // TSLA    
        64'h5350592020202020,  // SPY     
        64'h5151512020202020,  // QQQ     
        64'h474F4F474C202020,  // GOOGL   
        64'h4D53465420202020,  // MSFT    
        64'h414D5A4E20202020,  // AMZN    
        64'h4E56444120202020   // NVDA    
    };

    function automatic bit is_symbol_filtered(symbol_t symbol);
        if (!ENABLE_SYMBOL_FILTER) begin
            return 1'b1;
        end

        for (int i = 0; i < 8; ++i) begin
            if (symbol == FILTER_SYMBOL_LIST[i]) begin
                return 1'b1;
            end
        end

        return 1'b0;
    endfunction
endpackage
