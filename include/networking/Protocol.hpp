// include/networking/Protocol.hpp
#pragma once

#include "../engine/Types.hpp"
#include <string>
#include <vector>

namespace networking {

// Order submission request
struct OrderRequest {
    engine::OrderType type;
    engine::OrderSide side;
    std::string symbol;
    engine::Price price;
    engine::Quantity quantity;
    std::string clientOrderId;
};

// Order response
struct OrderResponse {
    engine::OrderId orderId;
    engine::OrderStatus status;
    std::string message;
    engine::Quantity filledQuantity;
    engine::Price averagePrice;
};

// Trade notification
struct TradeNotification {
    engine::TradeId tradeId;
    engine::OrderId buyOrderId;
    engine::OrderId sellOrderId;
    engine::Quantity quantity;
    engine::Price price;
    std::chrono::system_clock::time_point timestamp;
};

// Market data snapshot
struct MarketDataSnapshot {
    std::string symbol;
    std::chrono::system_clock::time_point timestamp;
    
    struct Level {
        engine::Price price;
        engine::Quantity quantity;
    };
    
    std::vector<Level> bids;
    std::vector<Level> asks;
    
    engine::Price lastPrice;
    engine::Quantity lastQuantity;
    engine::Quantity totalVolume;
};

// Protocol message types
enum class MessageType {
    ORDER_REQUEST = 1,
    ORDER_RESPONSE = 2,
    TRADE_NOTIFICATION = 3,
    MARKET_DATA_SNAPSHOT = 4,
    HEARTBEAT = 5
};

// Serialization functions (to be implemented)
std::string serializeOrderRequest(const OrderRequest& request);
OrderRequest deserializeOrderRequest(const std::string& data);

std::string serializeOrderResponse(const OrderResponse& response);
OrderResponse deserializeOrderResponse(const std::string& data);

std::string serializeTradeNotification(const TradeNotification& notification);
TradeNotification deserializeTradeNotification(const std::string& data);

} // namespace networking