// src/persistence/RedisStorage.cpp
#include "RedisStorage.hpp"
#include "../utils/Logger.hpp"
#include <hiredis/hiredis.h>
#include <sstream>

namespace persistence {

RedisStorage::RedisStorage(const std::string& host, int port, int db) 
    : host_(host), port_(port), db_(db), redisContext_(nullptr) {}

RedisStorage::~RedisStorage() {
    disconnect();
}

bool RedisStorage::connect() {
    redisContext_ = redisConnect(host_.c_str(), port_);
    if (redisContext_ == nullptr || redisContext_->err) {
        if (redisContext_) {
            LOG_ERROR("Redis connection error: {}", redisContext_->errstr);
            redisFree(redisContext_);
            redisContext_ = nullptr;
        } else {
            LOG_ERROR("Can't allocate Redis context");
        }
        return false;
    }
    
    // Select database
    auto reply = (redisReply*)redisCommand(redisContext_, "SELECT %d", db_);
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        LOG_ERROR("Failed to select Redis database: {}", 
                 reply ? reply->str : "Unknown error");
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    
    connected_ = true;
    LOG_INFO("Connected to Redis at {}:{} (db{})", host_, port_, db_);
    return true;
}

void RedisStorage::disconnect() {
    if (redisContext_) {
        redisFree(redisContext_);
        redisContext_ = nullptr;
    }
    connected_ = false;
}

bool RedisStorage::isConnected() const {
    return connected_;
}

bool RedisStorage::saveOrder(const engine::Order& order) {
    if (!connected_) return false;
    
    std::stringstream ss;
    ss << "HSET " << generateOrderKey(order.getId()) << " "
       << "user_id " << order.getUserId() << " "
       << "symbol " << order.getSymbol() << " "
       << "type " << static_cast<int>(order.getType()) << " "
       << "side " << static_cast<int>(order.getSide()) << " "
       << "price " << order.getPrice() << " "
       << "quantity " << order.getQuantity() << " "
       << "filled_quantity " << order.getFilledQuantity() << " "
       << "status " << static_cast<int>(order.getStatus()) << " "
       << "timestamp " << order.getTimestamp().count();
    
    return executeCommand(ss.str());
}

bool RedisStorage::saveTrade(const engine::Trade& trade) {
    if (!connected_) return false;
    
    std::stringstream ss;
    ss << "HSET " << generateTradeKey(trade) << " "
       << "buy_order_id " << trade.getBuyOrderId() << " "
       << "sell_order_id " << trade.getSellOrderId() << " "
       << "quantity " << trade.getQuantity() << " "
       << "price " << trade.getPrice() << " "
       << "timestamp " << trade.getTimestamp().count();
    
    // Also add to sorted set for time-based queries
    std::stringstream zaddCmd;
    zaddCmd << "ZADD trades:" << "SYMBOL" << " " 
            << trade.getTimestamp().count() << " " << generateTradeKey(trade);
    executeCommand(zaddCmd.str());
    
    return executeCommand(ss.str());
}

bool RedisStorage::saveOrderBookSnapshot(const std::string& symbol, 
                                        const engine::OrderBook& orderBook) {
    if (!connected_) return false;
    
    // This is a simplified implementation
    // In production, we'd serialize the entire order book state
    auto depth = orderBook.getDepth(10); // Save top 10 levels
    
    std::stringstream ss;
    ss << "HSET " << generateOrderBookKey(symbol) << " "
       << "timestamp " << std::chrono::duration_cast<std::chrono::milliseconds>(
           std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Save bids
    for (size_t i = 0; i < depth.bids.size(); ++i) {
        ss << " bid_price_" << i << " " << depth.bids[i].price
           << " bid_quantity_" << i << " " << depth.bids[i].totalQuantity;
    }
    
    // Save asks
    for (size_t i = 0; i < depth.asks.size(); ++i) {
        ss << " ask_price_" << i << " " << depth.asks[i].price
           << " ask_quantity_" << i << " " << depth.asks[i].totalQuantity;
    }
    
    return executeCommand(ss.str());
}

std::string RedisStorage::generateOrderKey(engine::OrderId orderId) const {
    return "order:" + std::to_string(orderId);
}

std::string RedisStorage::generateTradeKey(const engine::Trade& trade) const {
    return "trade:" + std::to_string(trade.getId());
}

std::string RedisStorage::generateOrderBookKey(const std::string& symbol) const {
    return "orderbook:" + symbol;
}

std::string RedisStorage::generatePositionKey(engine::UserId userId, const std::string& symbol) const {
    return "position:" + std::to_string(userId) + ":" + symbol;
}

bool RedisStorage::executeCommand(const std::string& command) {
    if (!connected_) return false;
    
    auto reply = (redisReply*)redisCommand(redisContext_, command.c_str());
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        LOG_ERROR("Redis command failed: {}", command);
        if (reply) {
            LOG_ERROR("Redis error: {}", reply->str);
            freeReplyObject(reply);
        }
        return false;
    }
    
    freeReplyObject(reply);
    return true;
}

} // namespace persistence