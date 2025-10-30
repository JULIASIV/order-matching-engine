// src/risk/RiskEngine.cpp
#include "RiskEngine.hpp"
#include "../utils/Logger.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace risk {

RiskEngine::RiskEngine(const utils::Config& config) : config_(config) {
    LOG_INFO("RiskEngine initialized");
}

RiskCheckResult RiskEngine::checkOrder(const engine::Order& order) {
    // Check order size limit
    auto sizeCheck = checkOrderSizeLimit(order.getUserId(), order.getQuantity());
    if (!sizeCheck.approved) {
        return sizeCheck;
    }
    
    // Check position limit
    auto positionCheck = checkPositionLimit(order.getUserId(), order.getSymbol(), 
                                          order.getSide(), order.getQuantity());
    if (!positionCheck.approved) {
        return positionCheck;
    }
    
    // Check notional limit
    double notional = order.getPrice() * order.getQuantity();
    auto notionalCheck = checkNotionalLimit(order.getUserId(), notional);
    if (!notionalCheck.approved) {
        return notionalCheck;
    }
    
    // Check daily volume limit
    auto volumeCheck = checkDailyVolumeLimit(order.getUserId(), order.getQuantity());
    if (!volumeCheck.approved) {
        return volumeCheck;
    }
    
    // Check drawdown limit
    auto drawdownCheck = checkDrawdownLimit(order.getUserId());
    if (!drawdownCheck.approved) {
        return drawdownCheck;
    }
    
    // Check price deviation (for market orders)
    if (order.getType() == engine::OrderType::MARKET) {
        auto priceCheck = checkPriceDeviation(order.getSymbol(), order.getPrice());
        if (!priceCheck.approved) {
            return priceCheck;
        }
    }
    
    return RiskCheckResult{true, "Approved", 0.0};
}

RiskCheckResult RiskEngine::checkPositionLimit(engine::UserId userId, const std::string& symbol, 
                                              engine::OrderSide side, int64_t quantity) {
    std::shared_lock lock(riskDataMutex_);
    
    auto userIt = userRiskData_.find(userId);
    if (userIt == userRiskData_.end()) {
        return RiskCheckResult{true, "No position data", 0.0};
    }
    
    auto& userData = userIt->second;
    std::shared_lock userLock(userData.mutex);
    
    auto posIt = userData.positions.find(symbol);
    int64_t currentPosition = (posIt != userData.positions.end()) ? posIt->second.netPosition : 0;
    
    int64_t newPosition = currentPosition + ((side == engine::OrderSide::BUY) ? quantity : -quantity);
    
    if (std::abs(newPosition) > userData.limits.maxPosition) {
        return RiskCheckResult{false, "Position limit exceeded", 
                              static_cast<double>(userData.limits.maxPosition)};
    }
    
    return RiskCheckResult{true, "Position check passed", 0.0};
}

RiskCheckResult RiskEngine::checkOrderSizeLimit(engine::UserId userId, int64_t orderSize) {
    std::shared_lock lock(riskDataMutex_);
    
    auto userIt = userRiskData_.find(userId);
    if (userIt == userRiskData_.end()) {
        return RiskCheckResult{true, "No user data", 0.0};
    }
    
    auto& userData = userIt->second;
    std::shared_lock userLock(userData.mutex);
    
    if (orderSize > userData.limits.maxOrderSize) {
        return RiskCheckResult{false, "Order size limit exceeded", 
                              static_cast<double>(userData.limits.maxOrderSize)};
    }
    
    return RiskCheckResult{true, "Order size check passed", 0.0};
}

void RiskEngine::recordTrade(const engine::Trade& trade) {
    // This would be called by the matching engine when a trade occurs
    // For now, we'll update positions based on the trade
    
    // In a real implementation, we'd have the user IDs from the orders
    // For demonstration, we'll use placeholder user IDs
    engine::UserId buyerId = 1; // Would come from buy order
    engine::UserId sellerId = 2; // Would come from sell order
    
    updatePosition(buyerId, "SYMBOL", engine::OrderSide::BUY, 
                  trade.getQuantity(), trade.getPrice());
    updatePosition(sellerId, "SYMBOL", engine::OrderSide::SELL, 
                  trade.getQuantity(), trade.getPrice());
    
    // Update daily volume
    {
        std::unique_lock lock(riskDataMutex_);
        if (auto it = userRiskData_.find(buyerId); it != userRiskData_.end()) {
            std::unique_lock userLock(it->second.mutex);
            it->second.dailyVolume += trade.getQuantity();
            it->second.dailyNotional += trade.getQuantity() * trade.getPrice();
        }
        if (auto it = userRiskData_.find(sellerId); it != userRiskData_.end()) {
            std::unique_lock userLock(it->second.mutex);
            it->second.dailyVolume += trade.getQuantity();
            it->second.dailyNotional += trade.getQuantity() * trade.getPrice();
        }
    }
}

void RiskEngine::updatePosition(engine::UserId userId, const std::string& symbol, 
                               engine::OrderSide side, int64_t quantity, double price) {
    std::unique_lock lock(riskDataMutex_);
    
    auto& userData = userRiskData_[userId];
    std::unique_lock userLock(userData.mutex);
    
    auto& position = userData.positions[symbol];
    position.symbol = symbol;
    
    if (side == engine::OrderSide::BUY) {
        position.netPosition += quantity;
        position.buyQuantity += quantity;
    } else {
        position.netPosition -= quantity;
        position.sellQuantity += quantity;
    }
    
    // Update notional value with current market price
    std::shared_lock priceLock(pricesMutex_);
    if (auto priceIt = marketPrices_.find(symbol); priceIt != marketPrices_.end()) {
        position.notionalValue = position.netPosition * priceIt->second;
        
        // Update unrealized PnL
        double avgPrice = (position.buyQuantity > 0 || position.sellQuantity > 0) ?
            (position.buyQuantity - position.sellQuantity) * price / position.netPosition : 0.0;
        position.unrealizedPnl = position.netPosition * (priceIt->second - avgPrice);
    }
}

void RiskEngine::updateMarketPrice(const std::string& symbol, double price) {
    std::unique_lock lock(pricesMutex_);
    marketPrices_[symbol] = price;
    
    // Update all positions with this symbol
    std::shared_lock riskLock(riskDataMutex_);
    for (auto& [userId, userData] : userRiskData_) {
        std::unique_lock userLock(userData.mutex);
        if (auto posIt = userData.positions.find(symbol); posIt != userData.positions.end()) {
            auto& position = posIt->second;
            position.notionalValue = position.netPosition * price;
            
            // Recalculate unrealized PnL
            // This is simplified - in reality, we'd track cost basis
            double avgPrice = (position.buyQuantity > 0 || position.sellQuantity > 0) ?
                std::abs(position.netPosition * price / position.netPosition) : 0.0;
            position.unrealizedPnl = position.netPosition * (price - avgPrice);
            
            updateEquity(userId, symbol, price);
        }
    }
}

double RiskEngine::calculateVar(engine::UserId userId, double confidenceLevel) const {
    std::shared_lock lock(riskDataMutex_);
    
    auto userIt = userRiskData_.find(userId);
    if (userIt == userRiskData_.end()) {
        return 0.0;
    }
    
    return calculatePortfolioVaR(userIt->second, confidenceLevel);
}

double RiskEngine::calculatePortfolioVaR(const UserRiskData& userData, double confidenceLevel) const {
    // Simplified VaR calculation
    // In production, this would use historical simulation or parametric methods
    if (userData.portfolioReturns.empty()) {
        return 0.0;
    }
    
    // For demonstration, using a simple standard deviation approach
    double sum = std::accumulate(userData.portfolioReturns.begin(), 
                                userData.portfolioReturns.end(), 0.0);
    double mean = sum / userData.portfolioReturns.size();
    
    double sq_sum = std::inner_product(userData.portfolioReturns.begin(), 
                                      userData.portfolioReturns.end(),
                                      userData.portfolioReturns.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / userData.portfolioReturns.size() - mean * mean);
    
    // Assuming normal distribution, 95% VaR is 1.645 standard deviations
    double zScore = (confidenceLevel == 0.95) ? 1.645 : 
                   (confidenceLevel == 0.99) ? 2.326 : 1.0;
    
    return zScore * stdev * userData.currentEquity;
}

} // namespace risk