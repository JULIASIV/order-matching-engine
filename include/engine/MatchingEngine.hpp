// include/engine/MatchingEngine.hpp
#pragma once

#include "OrderBook.hpp"
#include "Types.hpp"
#include "../networking/Protocol.hpp"
#include "../utils/LockFreeQueue.hpp"
#include "../utils/ThreadPool.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Config.hpp"
#include <atomic>
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <chrono>

namespace engine {

enum class EngineStatus {
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    ERROR
};

struct Statistics {
    uint64_t ordersProcessed{0};
    uint64_t tradesExecuted{0};
    uint64_t totalVolume{0};
    uint64_t avgLatencyNs{0};
    uint64_t maxLatencyNs{0};
};

class MatchingEngine {
public:
    MatchingEngine(const utils::Config& config);
    ~MatchingEngine();
    
    // Engine control
    void start();
    void stop();
    void shutdown();
    
    // Order management
    OrderResponse submitOrder(OrderPtr order);
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
    utils::Config config_;
    
    // Multi-threaded processing
    utils::ThreadPool processingPool_;
    utils::LockFreeQueue<OrderPtr, 100000> orderQueue_;
    utils::LockFreeQueue<OrderResponse, 100000> responseQueue_;
    
    std::atomic<bool> running_{false};
    std::atomic<EngineStatus> status_{EngineStatus::STOPPED};
    
    // Statistics
    mutable std::mutex statsMutex_;
    Statistics stats_;
    uint64_t totalProcessingTimeNs_{0};
    
    // Order ID generation
    std::atomic<OrderId> nextOrderId_{1};
    std::atomic<TradeId> nextTradeId_{1};
    
    void initializeInstruments();
    void processOrders();
    void processSingleOrder(OrderPtr order);
    void sendResponse(const OrderResponse& response);
    void updateStatistics(uint64_t processingTimeNs);
    
    // ID generation
    OrderId generateOrderId();
    TradeId generateTradeId();
};

} // namespace engine