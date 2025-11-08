// src/networking/FixAdapter.cpp
#include "FixAdapter.hpp"
#include "../utils/Logger.hpp"
#include <quickfix/FileStore.h>
#include <quickfix/SocketInitiator.h>
#include <quickfix/SessionSettings.h>

namespace networking {

FixAdapter::FixAdapter(std::shared_ptr<engine::MatchingEngine> engine, 
                       const std::string& configFile)
    : engine_(engine)
    , configFile_(configFile)
{
    LOG_INFO("FIX Adapter initialized with config: {}", configFile);
}

FixAdapter::~FixAdapter() {
    stop();
}

void FixAdapter::start() {
    if (running_.exchange(true)) {
        return;
    }
    
    try {
        FIX::SessionSettings settings(configFile_);
        FIX::FileStoreFactory storeFactory(settings);
        FIX::ScreenLogFactory logFactory(settings);
        
        initiator_ = std::make_unique<FIX::SocketInitiator>(*this, storeFactory, settings, logFactory);
        initiator_->start();
        
        LOG_INFO("FIX Adapter started successfully");
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to start FIX Adapter: {}", e.what());
        throw;
    }
}

void FixAdapter::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    if (initiator_) {
        initiator_->stop();
    }
    
    LOG_INFO("FIX Adapter stopped");
}

void FixAdapter::onCreate(const FIX::SessionID& sessionID) {
    LOG_INFO("FIX Session created: {}", sessionID.toString());
}

void FixAdapter::onLogon(const FIX::SessionID& sessionID) {
    LOG_INFO("FIX Session logon: {}", sessionID.toString());
}

void FixAdapter::onLogout(const FIX::SessionID& sessionID) {
    LOG_INFO("FIX Session logout: {}", sessionID.toString());
}

void FixAdapter::toAdmin(FIX::Message& message, const FIX::SessionID& sessionID) {
    logFIXMessage("OUT", message);
}

void FixAdapter::toApp(FIX::Message& message, const FIX::SessionID& sessionID) {
    logFIXMessage("OUT", message);
}

void FixAdapter::fromAdmin(const FIX::Message& message, const FIX::SessionID& sessionID) {
    logFIXMessage("IN", message);
    crack(message, sessionID);
}

void FixAdapter::fromApp(const FIX::Message& message, const FIX::SessionID& sessionID) {
    logFIXMessage("IN", message);
    crack(message, sessionID);
}

void FixAdapter::onMessage(const FIX42::NewOrderSingle& message, const FIX::SessionID& sessionID) {
    try {
        LOG_INFO("Received NewOrderSingle: {}", message.toString());
        
        // Extract FIX fields
        FIX::ClOrdID clOrdID;
        FIX::Symbol symbol;
        FIX::Side side;
        FIX::OrdType ordType;
        FIX::Price price;
        FIX::OrderQty orderQty;
        
        message.get(clOrdID);
        message.get(symbol);
        message.get(side);
        message.get(ordType);
        message.get(orderQty);
        
        if (ordType == FIX::OrdType_LIMIT || ordType == FIX::OrdType_STOP_LIMIT) {
            message.get(price);
        }
        
        // Convert to internal order
        auto order = std::make_shared<engine::Order>(
            engine_->generateOrderId(),
            1, // User ID from FIX session
            symbol.getValue(),
            fixToOrderType(ordType),
            fixToOrderSide(side),
            (ordType == FIX::OrdType_LIMIT || ordType == FIX::OrdType_STOP_LIMIT) ? price.getValue() : 0.0,
            orderQty.getValue()
        );
        
        // Store session for this order
        {
            std::unique_lock lock(sessionsMutex_);
            orderSessions_[order->getId()] = sessionID;
        }
        
        // Submit to matching engine
        auto response = engine_->submitOrder(order);
        
        // Send execution report
        sendExecutionReport(*order, sessionID);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error processing NewOrderSingle: {}", e.what());
    }
}

void FixAdapter::sendExecutionReport(const engine::Order& order, const FIX::SessionID& sessionID) {
    try {
        FIX42::ExecutionReport executionReport;
        
        executionReport.set(FIX::OrderID(std::to_string(order.getId())));
        executionReport.set(FIX::ExecID(std::to_string(engine_->generateTradeId())));
        executionReport.set(FIX::ExecTransType(FIX::ExecTransType_NEW));
        executionReport.set(FIX::ExecType(orderStatusToFix(order.getStatus())));
        executionReport.set(FIX::OrdStatus(orderStatusToFix(order.getStatus())));
        executionReport.set(FIX::Symbol(order.getSymbol()));
        executionReport.set(FIX::Side(orderSideToFix(order.getSide())));
        executionReport.set(FIX::OrderQty(order.getQuantity()));
        executionReport.set(FIX::LastQty(order.getFilledQuantity()));
        executionReport.set(FIX::LastPx(0.0)); // Would be actual fill price
        executionReport.set(FIX::LeavesQty(order.getRemainingQuantity()));
        executionReport.set(FIX::CumQty(order.getFilledQuantity()));
        executionReport.set(FIX::AvgPx(0.0)); // Would be average fill price
        
        executionReport.set(FIX::ClOrdID(std::to_string(order.getId())));
        executionReport.set(FIX::TransactTime(FIX::TransactTime()));
        
        FIX::Session::sendToTarget(executionReport, sessionID);
        logFIXMessage("OUT", executionReport);
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error sending execution report: {}", e.what());
    }
}

engine::OrderType FixAdapter::fixToOrderType(char fixOrdType) {
    switch (fixOrdType) {
        case FIX::OrdType_MARKET: return engine::OrderType::MARKET;
        case FIX::OrdType_LIMIT: return engine::OrderType::LIMIT;
        case FIX::OrdType_STOP: return engine::OrderType::STOP;
        case FIX::OrdType_STOP_LIMIT: return engine::OrderType::STOP_LIMIT;
        default: return engine::OrderType::LIMIT;
    }
}

engine::OrderSide FixAdapter::fixToOrderSide(char fixSide) {
    switch (fixSide) {
        case FIX::Side_BUY: return engine::OrderSide::BUY;
        case FIX::Side_SELL: return engine::OrderSide::SELL;
        default: return engine::OrderSide::BUY;
    }
}

char FixAdapter::orderTypeToFix(engine::OrderType type) {
    switch (type) {
        case engine::OrderType::MARKET: return FIX::OrdType_MARKET;
        case engine::OrderType::LIMIT: return FIX::OrdType_LIMIT;
        case engine::OrderType::FOK: return FIX::OrdType_LIMIT; // FIX doesn't have FOK directly
        case engine::OrderType::IOC: return FIX::OrdType_LIMIT; // FIX doesn't have IOC directly
        default: return FIX::OrdType_LIMIT;
    }
}

char FixAdapter::orderSideToFix(engine::OrderSide side) {
    return (side == engine::OrderSide::BUY) ? FIX::Side_BUY : FIX::Side_SELL;
}

char FixAdapter::orderStatusToFix(engine::OrderStatus status) {
    switch (status) {
        case engine::OrderStatus::NEW: return FIX::OrdStatus_NEW;
        case engine::OrderStatus::PARTIAL: return FIX::OrdStatus_PARTIALLY_FILLED;
        case engine::OrderStatus::FILLED: return FIX::OrdStatus_FILLED;
        case engine::OrderStatus::CANCELLED: return FIX::OrdStatus_CANCELED;
        case engine::OrderStatus::REJECTED: return FIX::OrdStatus_REJECTED;
        default: return FIX::OrdStatus_NEW;
    }
}

void FixAdapter::logFIXMessage(const std::string& direction, const FIX::Message& message) {
    LOG_DEBUG("FIX {}: {}", direction, message.toString());
}

} // namespace networking