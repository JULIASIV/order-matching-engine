// include/risk/RiskEngine.hpp
#pragma once

#include "../engine/Types.hpp"
#include "../utils/Config.hpp"
#include <unordered_map>
#include <shared_mutex>
#include <atomic>
#include <limits>

namespace risk {

struct RiskCheckResult {
    bool approved;
    std::string reason;
    double suggestedLimit;
};

struct Position {
    std::string symbol;
    int64_t netPosition{0};
    double notionalValue{0.0};
    int64_t buyQuantity{0};
    int64_t sellQuantity{0};
    double realizedPnl{0.0};
    double unrealizedPnl{0.0};
};

struct RiskLimits {
    int64_t maxPosition{10000};
    double maxNotional{1000000.0};
    int64_t maxOrderSize{1000};
    int64_t dailyVolumeLimit{1000000};
    double maxDrawdown{0.10}; // 10%
};

class RiskEngine {
public:
    RiskEngine(const utils::Config& config);
    
    RiskCheckResult checkOrder(const engine::Order& order);
    void recordTrade(const engine::Trade& trade);
    void updateMarketPrice(const std::string& symbol, double price);
    
    Position getPosition(engine::UserId userId, const std::string& symbol) const;
    std::unordered_map<std::string, Position> getAllPositions(engine::UserId userId) const;
    
    void setPositionLimit(engine::UserId userId, const std::string& symbol, int64_t limit);
    void setNotionalLimit(engine::UserId userId, double limit);
    void setDailyVolumeLimit(engine::UserId userId, int64_t limit);
    void setMaxOrderSize(engine::UserId userId, int64_t size);
    
    void resetDailyCounters();
    double calculateVar(engine::UserId userId, double confidenceLevel = 0.95) const;
    
private:
    struct UserRiskData {
        std::unordered_map<std::string, Position> positions;
        RiskLimits limits;
        int64_t dailyVolume{0};
        double dailyNotional{0.0};
        double startingEquity{1000000.0};
        double currentEquity{1000000.0};
        mutable std::shared_mutex mutex;
        
        std::vector<double> portfolioReturns; // For VaR calculation
    };
    
    std::unordered_map<engine::UserId, UserRiskData> userRiskData_;
    std::unordered_map<std::string, double> marketPrices_;
    mutable std::shared_mutex riskDataMutex_;
    mutable std::shared_mutex pricesMutex_;
    
    utils::Config config_;
    
    RiskCheckResult checkPositionLimit(engine::UserId userId, const std::string& symbol, 
                                      engine::OrderSide side, int64_t quantity);
    RiskCheckResult checkNotionalLimit(engine::UserId userId, double notionalValue);
    RiskCheckResult checkDailyVolumeLimit(engine::UserId userId, int64_t volume);
    RiskCheckResult checkOrderSizeLimit(engine::UserId userId, int64_t orderSize);
    RiskCheckResult checkDrawdownLimit(engine::UserId userId);
    RiskCheckResult checkPriceDeviation(const std::string& symbol, engine::Price price);
    
    void updatePosition(engine::UserId userId, const std::string& symbol, 
                       engine::OrderSide side, int64_t quantity, double price);
    void updateEquity(engine::UserId userId, const std::string& symbol, double newPrice);
    
    double calculatePortfolioVaR(const UserRiskData& userData, double confidenceLevel) const;
};

} // namespace risk