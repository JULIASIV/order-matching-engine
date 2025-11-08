// include/risk/CircuitBreaker.hpp
#pragma once

#include "../engine/Types.hpp"
#include <unordered_map>
#include <string>
#include <chrono>
#include <atomic>
#include <deque>

namespace risk {

class CircuitBreaker {
public:
    CircuitBreaker(const utils::Config& config);
    
    // Price movement checks
    bool checkPriceMove(const std::string& symbol, double newPrice);
    bool checkVolatility(const std::string& symbol, double currentVolatility);
    
    // Volume checks
    bool checkVolumeSpike(const std::string& symbol, int64_t volume);
    
    // Order rate checks
    bool checkOrderRate(const std::string& symbol, int ordersPerSecond);
    
    // Market-wide controls
    void triggerMarketWideHalt(const std::string& reason);
    void liftMarketWideHalt();
    bool isMarketHalted() const;
    
    // Symbol-specific controls
    void haltSymbol(const std::string& symbol, const std::string& reason);
    void resumeSymbol(const std::string& symbol);
    bool isSymbolHalted(const std::string& symbol) const;
    
    // Statistics
    struct MarketStats {
        double priceChangePercent{0.0};
        double volatility{0.0};
        int64_t volume{0};
        int orderRate{0};
        std::chrono::system_clock::time_point lastUpdate;
    };
    
    MarketStats getMarketStats(const std::string& symbol) const;
    
private:
    struct SymbolData {
        std::deque<double> priceHistory;
        std::deque<int64_t> volumeHistory;
        std::deque<std::chrono::system_clock::time_point> orderTimestamps;
        double referencePrice{0.0};
        bool halted{false};
        std::string haltReason;
        std::chrono::system_clock::time_point haltTime;
        
        // Limits
        double maxPriceMovePercent{0.10}; // 10%
        double maxVolatility{0.50}; // 50%
        int64_t maxVolumeSpike{1000000}; // 1M shares
        int maxOrderRate{1000}; // 1000 orders/second
    };
    
    std::unordered_map<std::string, SymbolData> symbolData_;
    mutable std::shared_mutex dataMutex_;
    
    std::atomic<bool> marketWideHalt_{false};
    std::string marketHaltReason_;
    std::chrono::system_clock::time_point marketHaltTime_;
    
    utils::Config config_;
    
    // Calculation methods
    double calculatePriceChange(const std::string& symbol, double newPrice);
    double calculateVolatility(const std::string& symbol);
    int64_t calculateVolumeSpike(const std::string& symbol, int64_t currentVolume);
    int calculateOrderRate(const std::string& symbol);
    
    // Configuration
    void loadConfiguration();
};

} // namespace risk