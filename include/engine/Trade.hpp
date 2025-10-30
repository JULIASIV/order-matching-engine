// include/engine/Trade.hpp
#pragma once

#include "Types.hpp"

namespace engine {

class Trade {
public:
    Trade(TradeId id, OrderId buyId, OrderId sellId, Quantity q, Price p, Timestamp ts)
        : tradeId(id), buyOrderId(buyId), sellOrderId(sellId), quantity(q), price(p), timestamp(ts)
    {}
    
    TradeId getId() const { return tradeId; }
    OrderId getBuyOrderId() const { return buyOrderId; }
    OrderId getSellOrderId() const { return sellOrderId; }
    Quantity getQuantity() const { return quantity; }
    Price getPrice() const { return price; }
    Timestamp getTimestamp() const { return timestamp; }
    
private:
    TradeId tradeId;
    OrderId buyOrderId;
    OrderId sellOrderId;
    Quantity quantity;
    Price price;
    Timestamp timestamp;
};

} // namespace engine