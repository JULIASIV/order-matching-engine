// include/engine/MatchingEngine.hpp
#pragma once

#include "OrderBook.hpp"
#include "../networking/Protocol.hpp"
#include "../persistence/StorageInterface.hpp"
#include "../risk/RiskEngine.hpp"
#include "../monitoring/Metrics.hpp"
#include "../utils/LockFreeQueue.hpp"
#include "../utils/ThreadPool.hpp"
#include "../utils/Clock.hpp"
#include <atomic>
#include <memory>
#include <vector>
#include <string>

namespace engine {

class MatchingEngine {
public:
    MatchingEngine(const std::string& configPath);
    ~MatchingEngine();
    
    // Disallow copying
    MatchingEngine(const MatchingEngine&) = delete;
    MatchingEngine& operator=(const MatchingEngine&) = delete;
    
    void start();
    void stop();
    void shutdown();
    
    // Order management
    OrderResponse submitOrder(Order order);
    bool cancelOrder(OrderId orderId, UserId userId);
    bool modifyOrder(OrderId orderId, UserId userId, Quantity newQuantity, Price newPrice);
    
    // Market data
    MarketDataSnapshot getMarketData(const std::string& symbol, uint8_t depth = 10) const;
    std::vector<Trade> getRecentTrades(const std::string& symbol, size_t count = 100) const;
    
    // Administration
    EngineStatus getStatus() const;
    Statistics getStatistics() const;
    void reloadConfiguration();
    
private:
    struct InstrumentData {
        OrderBook orderBook;
        std::vector<Trade> recentTrades;
        mutable std::shared_mutex mutex;
    };
    
    std::unordered_map<std::string, InstrumentData> instruments_;
    std::unique_ptr<RiskEngine> riskEngine_;
    std::unique_ptr<persistence::StorageInterface> persistence_;
    std::unique_ptr<monitoring::Metrics> metrics_;
    
    // Multi-threaded processing
    utils::ThreadPool processingPool_;
    utils::LockFreeQueue<Order> orderQueue_;
    utils::LockFreeQueue<OrderResponse> responseQueue_;
    
    std::atomic<bool> running_{false};
    std::atomic<EngineStatus> status_{EngineStatus::STOPPED};
    
    mutable std::shared_mutex configMutex_;
    utils::Config config_;
    
    // High-resolution clock for latency measurement
    utils::NanosecondClock clock_;
    
    void processOrders();
    void processSingleOrder(Order order);
    void validateOrder(Order& order);
    void persistOrder(const Order& order);
    void persistTrade(const Trade& trade);
    void sendResponse(const OrderResponse& response);
    void updateMetrics(const Order& order, const OrderResponse& response, 
                      uint64_t processingTimeNs);
    
    // Recovery and snapshotting
    void recoverFromSnapshot();
    void takeSnapshot();
    void scheduleSnapshot();
};

} // namespace engine