// include/engine/OrderBook.hpp
#pragma once

#include "Order.hpp"
#include "Trade.hpp"
#include <map>
#include <list>
#include <unordered_map>
#include <shared_mutex>
#include <vector>
#include <memory>

namespace engine {

class OrderBook {
public:
    OrderBook(std::string symbol);
    ~OrderBook() = default;
    
    // Order management
    std::vector<Trade> addOrder(OrderPtr order);
    bool cancelOrder(OrderId orderId);
    bool modifyOrder(OrderId orderId, Quantity newQuantity, Price newPrice);
    
    // Market data
    struct PriceLevel {
        Price price;
        Quantity totalQuantity;
        size_t orderCount;
    };
    
    struct Depth {
        std::vector<PriceLevel> bids;
        std::vector<PriceLevel> asks;
    };
    
    Depth getDepth(uint8_t levels = 10) const;
    std::vector<Trade> getRecentTrades(size_t count = 100) const;
    
    Price getBestBid() const;
    Price getBestAsk() const;
    Price getSpread() const;
    
    // Statistics
    Quantity getTotalVolume() const;
    size_t getTotalOrders() const;
    
private:
    struct OrderEntry {
        OrderPtr order;
        typename std::list<OrderPtr>::iterator iterator;
    };
    
    using OrderTree = std::map<Price, std::list<OrderPtr>, std::greater<Price>>;
    using OrderTreeAsk = std::map<Price, std::list<OrderPtr>>;
    
    std::string symbol_;
    OrderTree bids_;
    OrderTreeAsk asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;
    
    mutable std::shared_mutex mutex_;
    
    std::vector<Trade> recentTrades_;
    mutable std::mutex tradesMutex_;
    
    Quantity totalVolume_{0};
    size_t totalOrders_{0};
    
    // Matching algorithms
    std::vector<Trade> matchLimitOrder(OrderPtr order);
    std::vector<Trade> matchMarketOrder(OrderPtr order);
    std::vector<Trade> matchFOKOrder(OrderPtr order);
    std::vector<Trade> matchIOCOrder(OrderPtr order);
    std::vector<Trade> matchIcebergOrder(OrderPtr order);
    
    void executeTrade(OrderPtr buyOrder, OrderPtr sellOrder, 
                     Quantity quantity, Price price);
    void addToRecentTrades(const Trade& trade);
    
    // Utility functions
    void removeOrder(OrderId orderId);
    void updateOrderStatus(OrderPtr order, OrderStatus newStatus);
};

} // namespace engine