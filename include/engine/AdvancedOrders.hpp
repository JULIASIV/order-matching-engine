// include/engine/AdvancedOrders.hpp
#pragma once

#include "Order.hpp"
#include <functional>
#include <memory>

namespace engine {

// Iceberg Order - shows only a portion of the total quantity
class IcebergOrder : public Order {
public:
    IcebergOrder(OrderId id, UserId uid, std::string sym, OrderSide s, 
                 Price p, Quantity totalQty, Quantity peakSize,
                 Timestamp ts = std::chrono::steady_clock::now().time_since_epoch())
        : Order(id, uid, std::move(sym), OrderType::ICEBERG, s, p, totalQty, ts)
        , peakSize_(peakSize)
        , hiddenQuantity_(totalQty - std::min(peakSize, totalQty))
    {
        visibleQuantity_ = std::min(peakSize, totalQty);
    }
    
    Quantity getVisibleQuantity() const { return visibleQuantity_; }
    Quantity getHiddenQuantity() const { return hiddenQuantity_; }
    Quantity getPeakSize() const { return peakSize_; }
    
    void replenish() {
        if (hiddenQuantity_ > 0) {
            Quantity replenishAmount = std::min(peakSize_, hiddenQuantity_);
            visibleQuantity_ += replenishAmount;
            hiddenQuantity_ -= replenishAmount;
        }
    }
    
private:
    Quantity peakSize_;
    Quantity visibleQuantity_;
    Quantity hiddenQuantity_;
};

// Stop Order - becomes active when price reaches trigger level
class StopOrder : public Order {
public:
    StopOrder(OrderId id, UserId uid, std::string sym, OrderSide s, 
              Price triggerPrice, Price orderPrice, Quantity qty,
              Timestamp ts = std::chrono::steady_clock::now().time_since_epoch())
        : Order(id, uid, std::move(sym), OrderType::LIMIT, s, orderPrice, qty, ts)
        , triggerPrice_(triggerPrice)
        , activated_(false)
    {}
    
    bool shouldActivate(Price currentPrice) const {
        if (activated_) return false;
        
        if (side_ == OrderSide::BUY) {
            return currentPrice >= triggerPrice_;
        } else {
            return currentPrice <= triggerPrice_;
        }
    }
    
    void activate() {
        activated_ = true;
    }
    
    bool isActivated() const {
        return activated_;
    }
    
    Price getTriggerPrice() const {
        return triggerPrice_;
    }
    
private:
    Price triggerPrice_;
    bool activated_;
};

// TWAP Order - Time Weighted Average Price
class TWAPOrder : public Order {
public:
    TWAPOrder(OrderId id, UserId uid, std::string sym, OrderSide s,
              Price price, Quantity totalQty, 
              std::chrono::minutes duration, size_t slices,
              Timestamp ts = std::chrono::steady_clock::now().time_since_epoch())
        : Order(id, uid, std::move(sym), OrderType::LIMIT, s, price, totalQty, ts)
        , duration_(duration)
        , totalSlices_(slices)
        , currentSlice_(0)
        , sliceQuantity_(totalQty / slices)
    {}
    
    Quantity getSliceQuantity() const {
        return sliceQuantity_;
    }
    
    bool shouldExecuteSlice(std::chrono::system_clock::time_point currentTime) const {
        auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(
            currentTime - std::chrono::system_clock::time_point(timestamp_));
        
        auto targetSliceTime = (duration_ * currentSlice_) / totalSlices_;
        return elapsed >= targetSliceTime && currentSlice_ < totalSlices_;
    }
    
    void incrementSlice() {
        if (currentSlice_ < totalSlices_) {
            currentSlice_++;
        }
    }
    
    size_t getCurrentSlice() const {
        return currentSlice_;
    }
    
    size_t getTotalSlices() const {
        return totalSlices_;
    }
    
private:
    std::chrono::minutes duration_;
    size_t totalSlices_;
    size_t currentSlice_;
    Quantity sliceQuantity_;
};

} // namespace engine