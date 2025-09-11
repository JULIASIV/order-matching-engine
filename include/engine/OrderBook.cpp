// include/engine/OrderBook.hpp
#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include "Order.hpp"
#include "Types.hpp"

namespace engine {

class OrderBook {
public:
    OrderBook() = default;
    ~OrderBook() = default;
    
    // Delete copy operations
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
    
    // Allow move
    OrderBook(OrderBook&&) = default;
    OrderBook& operator=(OrderBook&&) = default;
    
    void addOrder(OrderPtr order);
    void cancelOrder(OrderId orderId);
    bool modifyOrder(OrderId orderId, Quantity newQuantity, Price newPrice);
    
    Depth getDepth(uint8_t levels = 10) const;
    std::vector<Trade> getRecentTrades(size_t count = 100) const;
    
    Price getBestBid() const;
    Price getBestAsk() const;
    Price getSpread() const;
    
private:
    using OrderTree = std::map<Price, std::list<OrderPtr>, std::greater<Price>>;
    using OrderTreeAsk = std::map<Price, std::list<OrderPtr>>;
    
    OrderTree bids_;
    OrderTreeAsk asks_;
    std::unordered_map<OrderId, OrderPtr> orders_;
    
    mutable std::shared_mutex mutex_;
    
    std::vector<Trade> recentTrades_;
    mutable std::mutex tradesMutex_;
    
    void matchOrder(OrderPtr order);
    void executeTrade(OrderPtr buyOrder, OrderPtr sellOrder, Quantity quantity, Price price);
    void addToRecentTrades(const Trade& trade);
};

} // namespace engine