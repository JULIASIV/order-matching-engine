// src/engine/OrderBook.cpp
#include "OrderBook.hpp"
#include "../utils/Logger.hpp"
#include <algorithm>
#include <numeric>

namespace engine {

OrderBook::OrderBook(std::string symbol) : symbol_(std::move(symbol)) {}

std::vector<Trade> OrderBook::addOrder(std::shared_ptr<Order> order) {
    std::unique_lock lock(mutex_);
    
    if (orders_.find(order->orderId) != orders_.end()) {
        LOG_WARNING("Order {} already exists in order book", order->orderId);
        return {};
    }
    
    totalOrders_++;
    
    switch (order->type) {
        case OrderType::LIMIT:
            return matchLimitOrder(order);
        case OrderType::MARKET:
            return matchMarketOrder(order);
        case OrderType::FOK:
            return matchFOKOrder(order);
        case OrderType::IOC:
            return matchIOCOrder(order);
        default:
            LOG_ERROR("Unknown order type: {}", static_cast<int>(order->type));
            return {};
    }
}

std::vector<Trade> OrderBook::matchLimitOrder(std::shared_ptr<Order> order) {
    std::vector<Trade> trades;
    
    if (order->side == OrderSide::BUY) {
        // Match against asks (sell orders)
        while (!asks_.empty() && order->getRemainingQuantity() > 0) {
            auto bestAsk = asks_.begin();
            if (bestAsk->first > order->price) {
                break; // No more matches possible
            }
            
            auto& ordersAtPrice = bestAsk->second;
            while (!ordersAtPrice.empty() && order->getRemainingQuantity() > 0) {
                auto matchingOrder = ordersAtPrice.front();
                
                Quantity tradeQuantity = std::min(
                    order->getRemainingQuantity(),
                    matchingOrder->getRemainingQuantity()
                );
                
                Price tradePrice = matchingOrder->price; // Price is set by existing order
                
                executeTrade(order, matchingOrder, tradeQuantity, tradePrice);
                trades.emplace_back(order->orderId, matchingOrder->orderId, 
                                  tradeQuantity, tradePrice, std::chrono::system_clock::now());
                
                if (matchingOrder->isFilled()) {
                    ordersAtPrice.pop_front();
                    removeOrder(matchingOrder->orderId);
                }
                
                if (ordersAtPrice.empty()) {
                    asks_.erase(bestAsk);
                    break;
                }
            }
        }
        
        // If there's remaining quantity, add to order book
        if (order->getRemainingQuantity() > 0) {
            auto& priceLevel = bids_[order->price];
            priceLevel.push_back(order);
            orders_[order->orderId] = {order, --priceLevel.end()};
            updateOrderStatus(order, OrderStatus::NEW);
        }
        
    } else { // SELL
        // Match against bids (buy orders)
        while (!bids_.empty() && order->getRemainingQuantity() > 0) {
            auto bestBid = bids_.begin();
            if (bestBid->first < order->price) {
                break; // No more matches possible
            }
            
            auto& ordersAtPrice = bestBid->second;
            while (!ordersAtPrice.empty() && order->getRemainingQuantity() > 0) {
                auto matchingOrder = ordersAtPrice.front();
                
                Quantity tradeQuantity = std::min(
                    order->getRemainingQuantity(),
                    matchingOrder->getRemainingQuantity()
                );
                
                Price tradePrice = matchingOrder->price; // Price is set by existing order
                
                executeTrade(matchingOrder, order, tradeQuantity, tradePrice);
                trades.emplace_back(matchingOrder->orderId, order->orderId, 
                                  tradeQuantity, tradePrice, std::chrono::system_clock::now());
                
                if (matchingOrder->isFilled()) {
                    ordersAtPrice.pop_front();
                    removeOrder(matchingOrder->orderId);
                }
                
                if (ordersAtPrice.empty()) {
                    bids_.erase(bestBid);
                    break;
                }
            }
        }
        
        // If there's remaining quantity, add to order book
        if (order->getRemainingQuantity() > 0) {
            auto& priceLevel = asks_[order->price];
            priceLevel.push_back(order);
            orders_[order->orderId] = {order, --priceLevel.end()};
            updateOrderStatus(order, OrderStatus::NEW);
        }
    }
    
    return trades;
}

std::vector<Trade> OrderBook::matchMarketOrder(std::shared_ptr<Order> order) {
    std::vector<Trade> trades;
    
    if (order->side == OrderSide::BUY) {
        // Match against asks (sell orders)
        while (!asks_.empty() && order->getRemainingQuantity() > 0) {
            auto bestAsk = asks_.begin();
            auto& ordersAtPrice = bestAsk->second;
            
            while (!ordersAtPrice.empty() && order->getRemainingQuantity() > 0) {
                auto matchingOrder = ordersAtPrice.front();
                
                Quantity tradeQuantity = std::min(
                    order->getRemainingQuantity(),
                    matchingOrder->getRemainingQuantity()
                );
                
                Price tradePrice = matchingOrder->price;
                
                executeTrade(order, matchingOrder, tradeQuantity, tradePrice);
                trades.emplace_back(order->orderId, matchingOrder->orderId, 
                                  tradeQuantity, tradePrice, std::chrono::system_clock::now());
                
                if (matchingOrder->isFilled()) {
                    ordersAtPrice.pop_front();
                    removeOrder(matchingOrder->orderId);
                }
                
                if (ordersAtPrice.empty()) {
                    asks_.erase(bestAsk);
                    break;
                }
            }
        }
        
        // Market orders are never added to the book
        if (order->getRemainingQuantity() > 0) {
            updateOrderStatus(order, OrderStatus::PARTIAL);
        } else {
            updateOrderStatus(order, OrderStatus::FILLED);
        }
        
    } else { // SELL
        // Match against bids (buy orders)
        while (!bids_.empty() && order->getRemainingQuantity() > 0) {
            auto bestBid = bids_.begin();
            auto& ordersAtPrice = bestBid->second;
            
            while (!ordersAtPrice.empty() && order->getRemainingQuantity() > 0) {
                auto matchingOrder = ordersAtPrice.front();
                
                Quantity tradeQuantity = std::min(
                    order->getRemainingQuantity(),
                    matchingOrder->getRemainingQuantity()
                );
                
                Price tradePrice = matchingOrder->price;
                
                executeTrade(matchingOrder, order, tradeQuantity, tradePrice);
                trades.emplace_back(matchingOrder->orderId, order->orderId, 
                                  tradeQuantity, tradePrice, std::chrono::system_clock::now());
                
                if (matchingOrder->isFilled()) {
                    ordersAtPrice.pop_front();
                    removeOrder(matchingOrder->orderId);
                }
                
                if (ordersAtPrice.empty()) {
                    bids_.erase(bestBid);
                    break;
                }
            }
        }
        
        // Market orders are never added to the book
        if (order->getRemainingQuantity() > 0) {
            updateOrderStatus(order, OrderStatus::PARTIAL);
        } else {
            updateOrderStatus(order, OrderStatus::FILLED);
        }
    }
    
    return trades;
}

void OrderBook::executeTrade(std::shared_ptr<Order> buyOrder, std::shared_ptr<Order> sellOrder, 
                            Quantity quantity, Price price) {
    buyOrder->filledQuantity += quantity;
    sellOrder->filledQuantity += quantity;
    
    totalVolume_ += quantity;
    
    if (buyOrder->isFilled()) {
        updateOrderStatus(buyOrder, OrderStatus::FILLED);
    } else {
        updateOrderStatus(buyOrder, OrderStatus::PARTIAL);
    }
    
    if (sellOrder->isFilled()) {
        updateOrderStatus(sellOrder, OrderStatus::FILLED);
    } else {
        updateOrderStatus(sellOrder, OrderStatus::PARTIAL);
    }
}

// Implementation of other methods...
} // namespace engine