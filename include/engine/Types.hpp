// include/engine/Types.hpp
#pragma once

#include <cstdint>
#include <string>
#include <chrono>
#include <memory>

namespace engine {

// Basic types
using OrderId = uint64_t;
using UserId = uint32_t;
using Quantity = int64_t;
using Price = double;
using Timestamp = std::chrono::nanoseconds;

// Order types
enum class OrderType {
    LIMIT,
    MARKET,
    FOK,    // Fill-or-Kill
    IOC,    // Immediate-or-Cancel
    ICEBERG
};

enum class OrderSide {
    BUY,
    SELL
};

enum class OrderStatus {
    NEW,
    PARTIAL,
    FILLED,
    CANCELLED,
    REJECTED,
    PENDING
};

// Forward declarations
class Order;
class Trade;
class OrderBook;
class MatchingEngine;

using OrderPtr = std::shared_ptr<Order>;
using TradePtr = std::shared_ptr<Trade>;

} // namespace engine