// src/engine/MatchingEngine.cpp (Enhanced)
#include "MatchingEngine.hpp"
#include "../risk/RiskEngine.hpp"
#include "../persistence/RedisStorage.hpp"
#include "../networking/Protocol.hpp"
#include <thread>
#include <algorithm>
#include <numeric>

namespace engine {

MatchingEngine::MatchingEngine(const utils::Config& config) 
    : config_(config)
    , processingPool_(config.get<int>("engine.processing_threads", 4))
{
    // Initialize risk engine
    riskEngine_ = std::make_unique<risk::RiskEngine>(config_);
    
    // Initialize persistence
    std::string redisHost = config.get<std::string>("persistence.redis_host", "localhost");
    int redisPort = config.get<int>("persistence.redis_port", 6379);
    int redisDb = config.get<int>("persistence.redis_db", 0);
    persistence_ = std::make_unique<persistence::RedisStorage>(redisHost, redisPort, redisDb);
    
    if (!persistence_->connect()) {
        LOG_ERROR("Failed to connect to Redis persistence");
    }
    
    initializeInstruments();
    LOG_INFO("MatchingEngine initialized with risk management and persistence");
}

void MatchingEngine::processSingleOrder(OrderPtr order) {
    // Risk check
    auto riskCheck = riskEngine_->checkOrder(*order);
    if (!riskCheck.approved) {
        OrderResponse response{order->getId(), OrderStatus::REJECTED, 
                              riskCheck.reason, 0, 0};
        sendResponse(response);
        return;
    }
    
    auto& instrument = instruments_.at(order->getSymbol());
    
    std::vector<Trade> trades;
    {
        std::unique_lock lock(instrument.mutex);
        trades = instrument.orderBook.addOrder(order);
        
        // Persist the order
        if (persistence_->isConnected()) {
            persistence_->saveOrder(*order);
        }
        
        // Process trades
        for (const auto& trade : trades) {
            instrument.recentTrades.push_back(trade);
            
            // Keep only the most recent trades
            if (instrument.recentTrades.size() > 1000) {
                instrument.recentTrades.erase(instrument.recentTrades.begin());
            }
            
            // Persist trade
            if (persistence_->isConnected()) {
                persistence_->saveTrade(trade);
            }
            
            // Update risk engine
            riskEngine_->recordTrade(trade);
        }
        
        // Save order book snapshot periodically
        static size_t orderCount = 0;
        if (++orderCount % 1000 == 0) { // Every 1000 orders
            if (persistence_->isConnected()) {
                persistence_->saveOrderBookSnapshot(order->getSymbol(), instrument.orderBook);
            }
            orderCount = 0;
        }
    }
    
    // Build and send response
    OrderResponse response = buildOrderResponse(order, trades);
    sendResponse(response);
    
    // Publish market data if needed
    if (!trades.empty()) {
        publishMarketData(order->getSymbol(), instrument.orderBook);
    }
}

OrderResponse MatchingEngine::buildOrderResponse(OrderPtr order, 
                                                const std::vector<Trade>& trades) {
    if (!trades.empty()) {
        const auto filledQuantity = std::accumulate(
            trades.begin(), trades.end(), Quantity(0),
            [](Quantity total, const Trade& trade) { return total + trade.getQuantity(); });
        
        const auto avgPrice = std::accumulate(
            trades.begin(), trades.end(), 0.0,
            [](double total, const Trade& trade) { return total + trade.getPrice(); }) / trades.size();
        
        if (filledQuantity == order->getQuantity()) {
            return OrderResponse{order->getId(), OrderStatus::FILLED, 
                               "", filledQuantity, avgPrice};
        } else {
            return OrderResponse{order->getId(), OrderStatus::PARTIAL, 
                               "", filledQuantity, avgPrice};
        }
    } else {
        if (order->getType() == OrderType::IOC || order->getType() == OrderType::FOK) {
            return OrderResponse{order->getId(), OrderStatus::CANCELLED, 
                               "Order not filled", 0, 0};
        } else {
            return OrderResponse{order->getId(), OrderStatus::NEW, "", 0, 0};
        }
    }
}

void MatchingEngine::publishMarketData(const std::string& symbol, 
                                      const OrderBook& orderBook) {
    // This would publish market data via ZeroMQ
    // For now, we'll just log it
    auto depth = orderBook.getDepth(5);
    
    LOG_DEBUG("Market data - {}: Best Bid={}, Best Ask={}, Spread={}", 
             symbol, orderBook.getBestBid(), orderBook.getBestAsk(), 
             orderBook.getSpread());
    
    // Update risk engine with latest prices
    if (!depth.bids.empty()) {
        riskEngine_->updateMarketPrice(symbol, depth.bids[0].price);
    }
}

} // namespace engine