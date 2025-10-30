// include/persistence/RedisStorage.hpp
#pragma once

#include "../engine/Types.hpp"
#include <string>
#include <memory>
#include <unordered_map>

namespace persistence {

class RedisStorage {
public:
    RedisStorage(const std::string& host = "localhost", int port = 6379, int db = 0);
    ~RedisStorage();
    
    bool connect();
    void disconnect();
    bool isConnected() const;
    
    // Order persistence
    bool saveOrder(const engine::Order& order);
    bool updateOrder(const engine::Order& order);
    std::shared_ptr<engine::Order> loadOrder(engine::OrderId orderId);
    bool deleteOrder(engine::OrderId orderId);
    
    // Trade persistence
    bool saveTrade(const engine::Trade& trade);
    std::vector<engine::Trade> loadTrades(const std::string& symbol, 
                                         size_t limit = 1000,
                                         const std::string& startTime = "",
                                         const std::string& endTime = "");
    
    // Order book snapshot
    bool saveOrderBookSnapshot(const std::string& symbol, 
                              const engine::OrderBook& orderBook);
    bool loadOrderBookSnapshot(const std::string& symbol,
                              engine::OrderBook& orderBook);
    
    // Market data
    bool saveMarketData(const std::string& symbol, double price, double volume);
    std::vector<std::tuple<double, double, std::string>> getMarketDataHistory(
        const std::string& symbol, size_t limit = 1000);
    
    // Risk data
    bool savePosition(const risk::Position& position, engine::UserId userId);
    risk::Position loadPosition(engine::UserId userId, const std::string& symbol);
    std::unordered_map<std::string, risk::Position> loadAllPositions(engine::UserId userId);
    
private:
    std::string host_;
    int port_;
    int db_;
    void* redisContext_;
    bool connected_{false};
    
    std::string generateOrderKey(engine::OrderId orderId) const;
    std::string generateTradeKey(const engine::Trade& trade) const;
    std::string generateOrderBookKey(const std::string& symbol) const;
    std::string generatePositionKey(engine::UserId userId, const std::string& symbol) const;
    
    bool executeCommand(const std::string& command);
    std::string executeCommandWithReply(const std::string& command);
};

} // namespace persistence