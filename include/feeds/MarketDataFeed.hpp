// include/feeds/MarketDataFeed.hpp
#pragma once

#include "../engine/Types.hpp"
#include "../networking/ZmqInterface.hpp"
#include <memory>
#include <unordered_map>
#include <thread>
#include <atomic>

namespace feeds {

class MarketDataFeed {
public:
    MarketDataFeed(std::shared_ptr<networking::ZmqInterface> zmqInterface,
                   const std::string& feedConfig);
    ~MarketDataFeed();
    
    void start();
    void stop();
    
    void subscribe(const std::string& symbol);
    void unsubscribe(const std::string& symbol);
    
    // Callbacks for different market data types
    virtual void onQuote(const std::string& symbol, double bid, double ask, 
                        int64_t bidSize, int64_t askSize) = 0;
    virtual void onTrade(const std::string& symbol, double price, 
                        int64_t quantity, const std::string& timestamp) = 0;
    virtual void onDepthUpdate(const std::string& symbol, 
                              const std::vector<std::pair<double, int64_t>>& bids,
                              const std::vector<std::pair<double, int64_t>>& asks) = 0;
    
protected:
    std::shared_ptr<networking::ZmqInterface> zmqInterface_;
    std::unordered_map<std::string, bool> subscriptions_;
    mutable std::shared_mutex subscriptionsMutex_;
    std::atomic<bool> running_{false};
    std::thread feedThread_;
    
private:
    virtual void runFeed() = 0;
};

} // namespace feeds