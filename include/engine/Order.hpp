// include/engine/Order.hpp
#pragma once

#include "Types.hpp"
#include <limits>

namespace engine {

class Order {
public:
    Order(OrderId id, UserId uid, std::string sym, OrderType t, OrderSide s, 
          Price p, Quantity q, Timestamp ts = std::chrono::steady_clock::now().time_since_epoch())
        : orderId(id), userId(uid), symbol(std::move(sym)), type(t), side(s), 
          price(p), quantity(q), timestamp(ts) 
    {
        if (type == OrderType::MARKET) {
            price = (side == OrderSide::BUY) ? std::numeric_limits<Price>::max() 
                                            : std::numeric_limits<Price>::min();
        }
    }
    
    // Getters
    OrderId getId() const { return orderId; }
    UserId getUserId() const { return userId; }
    const std::string& getSymbol() const { return symbol; }
    OrderType getType() const { return type; }
    OrderSide getSide() const { return side; }
    Price getPrice() const { return price; }
    Quantity getQuantity() const { return quantity; }
    Quantity getFilledQuantity() const { return filledQuantity; }
    Timestamp getTimestamp() const { return timestamp; }
    OrderStatus getStatus() const { return status; }
    
    // State management
    Quantity getRemainingQuantity() const {
        return quantity - filledQuantity;
    }
    
    bool isFilled() const {
        return filledQuantity >= quantity;
    }
    
    bool isActive() const {
        return status == OrderStatus::NEW || status == OrderStatus::PARTIAL;
    }
    
    void setFilledQuantity(Quantity qty) {
        filledQuantity = qty;
        if (isFilled()) {
            status = OrderStatus::FILLED;
        } else if (filledQuantity > 0) {
            status = OrderStatus::PARTIAL;
        }
    }
    
    void setStatus(OrderStatus newStatus) {
        status = newStatus;
    }
    
private:
    OrderId orderId;
    UserId userId;
    std::string symbol;
    OrderType type;
    OrderSide side;
    Price price;
    Quantity quantity;
    Quantity filledQuantity{0};
    Timestamp timestamp;
    OrderStatus status{OrderStatus::NEW};
    
    // For iceberg orders
    Quantity visibleQuantity{0};
    Quantity peakSize{0};
};

} // namespace engine