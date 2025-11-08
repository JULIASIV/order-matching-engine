// include/networking/FixAdapter.hpp
#pragma once

#include "../engine/Types.hpp"
#include "quickfix/Application.h"
#include "quickfix/MessageCracker.h"
#include "quickfix/Values.h"
#include <memory>
#include <unordered_map>

namespace networking {

class FixAdapter : public FIX::Application, public FIX::MessageCracker {
public:
    FixAdapter(std::shared_ptr<engine::MatchingEngine> engine, 
               const std::string& configFile);
    ~FixAdapter();
    
    void start();
    void stop();
    
    // FIX::Application methods
    void onCreate(const FIX::SessionID&) override;
    void onLogon(const FIX::SessionID&) override;
    void onLogout(const FIX::SessionID&) override;
    void toAdmin(FIX::Message&, const FIX::SessionID&) override;
    void toApp(FIX::Message&, const FIX::SessionID&) override;
    void fromAdmin(const FIX::Message&, const FIX::SessionID&) override;
    void fromApp(const FIX::Message&, const FIX::SessionID&) override;
    
    // FIX message handlers
    void onMessage(const FIX42::NewOrderSingle& message, const FIX::SessionID& sessionID);
    void onMessage(const FIX42::OrderCancelRequest& message, const FIX::SessionID& sessionID);
    void onMessage(const FIX42::OrderCancelReplaceRequest& message, const FIX::SessionID& sessionID);
    void onMessage(const FIX42::OrderStatusRequest& message, const FIX::SessionID& sessionID);
    
    // Send FIX messages
    void sendExecutionReport(const engine::Order& order, const FIX::SessionID& sessionID);
    void sendOrderCancelReject(const FIX42::OrderCancelRequest& request, 
                              const FIX::SessionID& sessionID, const std::string& reason);
    void sendMarketDataSnapshot(const std::string& symbol, const engine::OrderBook::Depth& depth);
    
private:
    std::shared_ptr<engine::MatchingEngine> engine_;
    std::unique_ptr<FIX::SocketInitiator> initiator_;
    std::string configFile_;
    std::atomic<bool> running_{false};
    
    std::unordered_map<engine::OrderId, FIX::SessionID> orderSessions_;
    mutable std::shared_mutex sessionsMutex_;
    
    // FIX message construction
    FIX42::NewOrderSingle createNewOrderSingle(const engine::Order& order);
    FIX42::ExecutionReport createExecutionReport(const engine::Order& order, char execType);
    FIX42::MarketDataSnapshotFullRefresh createMarketDataSnapshot(const std::string& symbol, 
                                                                 const engine::OrderBook::Depth& depth);
    
    // Utility methods
    engine::OrderType fixToOrderType(char fixOrdType);
    engine::OrderSide fixToOrderSide(char fixSide);
    char orderTypeToFix(engine::OrderType type);
    char orderSideToFix(engine::OrderSide side);
    char orderStatusToFix(engine::OrderStatus status);
    
    void logFIXMessage(const std::string& direction, const FIX::Message& message);
};

} // namespace networking