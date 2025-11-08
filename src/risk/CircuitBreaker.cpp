// src/risk/CircuitBreaker.cpp
#include "CircuitBreaker.hpp"
#include "../utils/Logger.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace risk {

CircuitBreaker::CircuitBreaker(const utils::Config& config) : config_(config) {
    loadConfiguration();
    LOG_INFO("Circuit Breaker initialized");
}

void CircuitBreaker::loadConfiguration() {
    // Load circuit breaker limits from config
    // This would typically read from config file
}

bool CircuitBreaker::checkPriceMove(const std::string& symbol, double newPrice) {
    std::unique_lock lock(dataMutex_);
    
    auto& data = symbolData_[symbol];
    
    if (data.priceHistory.empty()) {
        data.priceHistory.push_back(newPrice);
        data.referencePrice = newPrice;
        return true;
    }
    
    double priceChange = calculatePriceChange(symbol, newPrice);
    data.priceHistory.push_back(newPrice);
    
    // Keep only recent prices (e.g., last 100)
    if (data.priceHistory.size() > 100) {
        data.priceHistory.pop_front();
    }
    
    if (std::abs(priceChange) > data.maxPriceMovePercent) {
        LOG_WARNING("Circuit breaker triggered for {}: price moved {:.2f}%", 
                   symbol, priceChange * 100);
        haltSymbol(symbol, "Price movement limit exceeded");
        return false;
    }
    
    return true;
}

bool CircuitBreaker::checkVolatility(const std::string& symbol, double currentVolatility) {
    std::unique_lock lock(dataMutex_);
    
    auto& data = symbolData_[symbol];
    
    if (currentVolatility > data.maxVolatility) {
        LOG_WARNING("Circuit breaker triggered for {}: volatility {:.2f}% exceeded limit", 
                   symbol, currentVolatility * 100);
        haltSymbol(symbol, "Volatility limit exceeded");
        return false;
    }
    
    return true;
}

bool CircuitBreaker::checkVolumeSpike(const std::string& symbol, int64_t volume) {
    std::unique_lock lock(dataMutex_);
    
    auto& data = symbolData_[symbol];
    data.volumeHistory.push_back(volume);
    
    // Keep only recent volumes (e.g., last 50)
    if (data.volumeHistory.size() > 50) {
        data.volumeHistory.pop_front();
    }
    
    int64_t volumeSpike = calculateVolumeSpike(symbol, volume);
    
    if (volumeSpike > data.maxVolumeSpike) {
        LOG_WARNING("Circuit breaker triggered for {}: volume spike {} exceeded limit", 
                   symbol, volumeSpike);
        haltSymbol(symbol, "Volume spike detected");
        return false;
    }
    
    return true;
}

bool CircuitBreaker::checkOrderRate(const std::string& symbol, int ordersPerSecond) {
    std::unique_lock lock(dataMutex_);
    
    auto& data = symbolData_[symbol];
    auto now = std::chrono::system_clock::now();
    data.orderTimestamps.push_back(now);
    
    // Remove timestamps older than 1 second
    auto oneSecondAgo = now - std::chrono::seconds(1);
    while (!data.orderTimestamps.empty() && data.orderTimestamps.front() < oneSecondAgo) {
        data.orderTimestamps.pop_front();
    }
    
    int currentOrderRate = data.orderTimestamps.size();
    
    if (currentOrderRate > data.maxOrderRate) {
        LOG_WARNING("Circuit breaker triggered for {}: order rate {} exceeded limit", 
                   symbol, currentOrderRate);
        haltSymbol(symbol, "Order rate limit exceeded");
        return false;
    }
    
    return true;
}

void CircuitBreaker::haltSymbol(const std::string& symbol, const std::string& reason) {
    std::unique_lock lock(dataMutex_);
    
    auto& data = symbolData_[symbol];
    data.halted = true;
    data.haltReason = reason;
    data.haltTime = std::chrono::system_clock::now();
    
    LOG_ERROR("Symbol {} halted: {}", symbol, reason);
}

void CircuitBreaker::resumeSymbol(const std::string& symbol) {
    std::unique_lock lock(dataMutex_);
    
    auto& data = symbolData_[symbol];
    data.halted = false;
    data.haltReason.clear();
    
    LOG_INFO("Symbol {} resumed", symbol);
}

bool CircuitBreaker::isSymbolHalted(const std::string& symbol) const {
    std::shared_lock lock(dataMutex_);
    
    auto it = symbolData_.find(symbol);
    if (it == symbolData_.end()) {
        return false;
    }
    
    return it->second.halted;
}

double CircuitBreaker::calculatePriceChange(const std::string& symbol, double newPrice) {
    auto& data = symbolData_[symbol];
    
    if (data.referencePrice == 0.0) {
        return 0.0;
    }
    
    return (newPrice - data.referencePrice) / data.referencePrice;
}

double CircuitBreaker::calculateVolatility(const std::string& symbol) {
    auto& data = symbolData_[symbol];
    
    if (data.priceHistory.size() < 2) {
        return 0.0;
    }
    
    // Calculate standard deviation of returns
    std::vector<double> returns;
    for (size_t i = 1; i < data.priceHistory.size(); ++i) {
        double ret = (data.priceHistory[i] - data.priceHistory[i-1]) / data.priceHistory[i-1];
        returns.push_back(ret);
    }
    
    double mean = std::accumulate(returns.begin(), returns.end(), 0.0) / returns.size();
    double sq_sum = std::inner_product(returns.begin(), returns.end(), returns.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / returns.size() - mean * mean);
    
    return stdev * std::sqrt(252); // Annualized volatility
}

int64_t CircuitBreaker::calculateVolumeSpike(const std::string& symbol, int64_t currentVolume) {
    auto& data = symbolData_[symbol];
    
    if (data.volumeHistory.size() < 2) {
        return currentVolume;
    }
    
    int64_t averageVolume = std::accumulate(data.volumeHistory.begin(), 
                                           data.volumeHistory.end(), 0LL) / data.volumeHistory.size();
    
    return currentVolume - averageVolume;
}

} // namespace risk