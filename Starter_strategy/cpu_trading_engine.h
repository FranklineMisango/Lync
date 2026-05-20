#pragma once

#include <cstdint>
#include <unordered_map>

// CPU-based trading engine - pure software equivalent to FPGA alpha_core
struct TradeDecision {
    bool should_send;
    bool order_side;
    uint32_t order_price;
    uint32_t order_qty;
};

class CPUTradingEngine {
public:
    static constexpr uint16_t TARGET_LOCATE = 1337;  // AAPL
    static constexpr uint32_t MAX_POSITION = 5000;
    static constexpr int IMBALANCE_ALPHA = 3;

    struct MarketState {
        uint64_t bid_depth = 0;
        uint64_t ask_depth = 0;
        int32_t position = 0;
        uint32_t last_bid_price = 0;
        uint32_t last_ask_price = 0;
    };

    struct OrderRecord {
        uint8_t side;
        uint32_t price;
        uint32_t shares;
    };

    CPUTradingEngine() : market_state(), order_book() {}

    // Process ITCH message and make trading decision
    TradeDecision process_itch_message(
        uint8_t msg_type,
        uint16_t msg_locate,
        uint8_t msg_side,
        uint32_t msg_price,
        uint32_t msg_shares,
        uint64_t order_id
    ) {
        TradeDecision decision = {false, false, 0, 0};

        // Filter to target security
        if (msg_locate != TARGET_LOCATE) {
            return decision;
        }

        switch (msg_type) {
            case 'A':  // Add Order
                return process_add_order(msg_side, msg_price, msg_shares, order_id);
            
            case 'D':  // Delete Order
                return process_delete_order(order_id);
            
            case 'X':  // Cancel Order
                return process_cancel_order(order_id, msg_shares);
            
            case 'E':  // Execute Order
                return process_execute_order(order_id, msg_shares);
            
            default:
                return decision;
        }
    }

    const MarketState& get_market_state() const {
        return market_state;
    }

private:
    MarketState market_state;
    std::unordered_map<uint64_t, OrderRecord> order_book;

    TradeDecision process_add_order(uint8_t side, uint32_t price, uint32_t shares, uint64_t order_id) {
        TradeDecision decision = {false, false, 0, 0};

        // Record order
        order_book[order_id] = {side, price, shares};

        // Update market depth
        if (side == 1) {  // Buy
            market_state.bid_depth += shares;
            market_state.last_bid_price = price;
        } else {  // Sell
            market_state.ask_depth += shares;
            market_state.last_ask_price = price;
        }

        // Alpha Signal: Order Imbalance
        int64_t imbalance = (int64_t)market_state.bid_depth - (int64_t)market_state.ask_depth;
        
        // If bid side significantly larger than ask, sell (contraposition)
        if (imbalance > (shares * IMBALANCE_ALPHA) && market_state.position < (int32_t)MAX_POSITION) {
            decision.should_send = true;
            decision.order_side = 0;  // Sell
            decision.order_price = price - 100;  // Sell 1 cent lower
            decision.order_qty = std::min(uint32_t(100), shares / 2);
            market_state.position += decision.order_qty;
        }
        // If ask side significantly larger than bid, buy (contraposition)
        else if (imbalance < -(shares * IMBALANCE_ALPHA) && market_state.position > -(int32_t)MAX_POSITION) {
            decision.should_send = true;
            decision.order_side = 1;  // Buy
            decision.order_price = price + 100;  // Buy 1 cent higher
            decision.order_qty = std::min(uint32_t(100), shares / 2);
            market_state.position -= decision.order_qty;
        }

        return decision;
    }

    TradeDecision process_delete_order(uint64_t order_id) {
        TradeDecision decision = {false, false, 0, 0};
        
        auto it = order_book.find(order_id);
        if (it != order_book.end()) {
            const auto& order = it->second;
            if (order.side == 1) {  // Buy
                market_state.bid_depth = (market_state.bid_depth >= order.shares) 
                    ? market_state.bid_depth - order.shares : 0;
            } else {  // Sell
                market_state.ask_depth = (market_state.ask_depth >= order.shares) 
                    ? market_state.ask_depth - order.shares : 0;
            }
            order_book.erase(it);
        }
        
        return decision;
    }

    TradeDecision process_cancel_order(uint64_t order_id, uint32_t cancelled_shares) {
        TradeDecision decision = {false, false, 0, 0};
        
        auto it = order_book.find(order_id);
        if (it != order_book.end()) {
            auto& order = it->second;
            if (order.shares <= cancelled_shares) {
                if (order.side == 1) {  // Buy
                    market_state.bid_depth = (market_state.bid_depth >= order.shares) 
                        ? market_state.bid_depth - order.shares : 0;
                } else {  // Sell
                    market_state.ask_depth = (market_state.ask_depth >= order.shares) 
                        ? market_state.ask_depth - order.shares : 0;
                }
                order_book.erase(it);
            } else {
                order.shares -= cancelled_shares;
                if (order.side == 1) {  // Buy
                    market_state.bid_depth = (market_state.bid_depth >= cancelled_shares) 
                        ? market_state.bid_depth - cancelled_shares : 0;
                } else {  // Sell
                    market_state.ask_depth = (market_state.ask_depth >= cancelled_shares) 
                        ? market_state.ask_depth - cancelled_shares : 0;
                }
            }
        }
        
        return decision;
    }

    TradeDecision process_execute_order(uint64_t order_id, uint32_t executed_shares) {
        return process_cancel_order(order_id, executed_shares);
    }
};
