// include/risk/RiskEngine.hpp
#pragma once

#include "../engine/Types.hpp"
#include "../utils/Config.hpp"
#include <unordered_map>
#include <shared_mutex>
#include <atomic>

namespace risk {

struct RiskCheckResult {
    bool approved;
    std::string reason;
};

struct Position {
    std::string symbol;
    int64_t netPosition{0};
    double notionalValue{0.0};
    int64_t buyQuantity{0};
    int64_t sellQuantity{0};
};

class RiskEngine {
public:
    RiskEngine(const utils::Config& config);
    
    RiskCheckResult checkOrder(const engine::Order& order);
    void recordTrade(const engine::Trade& trade);
    
    Position getPosition(UserId userId, const std::string& symbol) const;
    std::unordered_map<std::string, Position> getAllPositions(UserId userId) const;
    
    void setPositionLimit(UserId userId, const std::string& symbol, int64_t limit);
    void setNotionalLimit(UserId userId, double limit);
    void setDailyVolumeLimit(UserId userId, int64_t limit);
    
    void resetDailyCounters();
    
private:
    struct UserRiskData {
        std::unordered_map<std::string, Position> positions;
        double notionalLimit{1000000.0}; // Default 1M
        int64_t dailyVolumeLimit{1000000}; // Default 1M shares
        int64_t dailyVolume{0};
        mutable std::shared_mutex mutex;
    };
    
    std::unordered_map<UserId, UserRiskData> userRiskData_;
    mutable std::shared_mutex riskDataMutex_;
    
    utils::Config config_;
    
    RiskCheckResult checkPositionLimit(UserId userId, const std::string& symbol, 
                                      engine::OrderSide side, int64_t quantity);
    RiskCheckResult checkNotionalLimit(UserId userId, double notionalValue);
    RiskCheckResult checkDailyVolumeLimit(UserId userId, int64_t volume);
    RiskCheckResult checkPriceDeviation(const std::string& symbol, engine::Price price);
};

} // namespace risk