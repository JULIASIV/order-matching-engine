// src/engine/MatchingEngine.cpp
#include "MatchingEngine.hpp"
#include "../networking/Protocol.hpp"
#include "../persistence/RedisStorage.hpp"
#include "../risk/RiskEngine.hpp"
#include "../monitoring/Metrics.hpp"
#include "../utils/Logger.hpp"
#include <chrono>
#include <thread>
#include <fstream>

namespace engine {

MatchingEngine::MatchingEngine(const std::string& configPath) 
    : config_(configPath)
    , processingPool_(config_.get<int>("engine.threads", 4))
    , orderQueue_(config_.get<size_t>("engine.queue_size", 100000))
    , responseQueue_(config_.get<size_t>("engine.response_queue_size", 100000))
{
    // Initialize components
    riskEngine_ = std::make_unique<RiskEngine>(config_);
    persistence_ = std::make_unique<persistence::RedisStorage>(config_);
    metrics_ = std::make_unique<monitoring::Metrics>(config_);
    
    // Load instrument list
    auto instruments = config_.get<std::vector<std::string>>("instruments", {});
    for (const auto& symbol : instruments) {
        instruments_.emplace(symbol, InstrumentData{});
    }
    
    LOG_INFO("MatchingEngine initialized with {} instruments", instruments.size());
}

MatchingEngine::~MatchingEngine() {
    shutdown();
}

void MatchingEngine::start() {
    if (running_.exchange(true)) {
        LOG_WARNING("MatchingEngine already running");
        return;
    }
    
    status_ = EngineStatus::STARTING;
    
    // Recover from snapshot
    recoverFromSnapshot();
    
    // Start processing threads
    processingPool_.start();
    
    // Start order processing
    for (int i = 0; i < config_.get<int>("engine.processing_threads", 2); ++i) {
        processingPool_.submit([this] { processOrders(); });
    }
    
    // Schedule periodic tasks
    processingPool_.submit([this] { scheduleSnapshot(); });
    
    status_ = EngineStatus::RUNNING;
    LOG_INFO("MatchingEngine started successfully");
}

void MatchingEngine::stop() {
    if (!running_.exchange(false)) {
        LOG_WARNING("MatchingEngine already stopped");
        return;
    }
    
    status_ = EngineStatus::STOPPING;
    
    // Take final snapshot
    takeSnapshot();
    
    // Stop processing
    processingPool_.stop();
    
    status_ = EngineStatus::STOPPED;
    LOG_INFO("MatchingEngine stopped successfully");
}

void MatchingEngine::shutdown() {
    stop();
    // Additional cleanup if needed
}

OrderResponse MatchingEngine::submitOrder(Order order) {
    if (!running_.load()) {
        return OrderResponse{order.orderId, OrderStatus::REJECTED, 
                            "Engine not running", 0, 0};
    }
    
    const auto startTime = clock_.now();
    
    try {
        // Validate order basics
        if (order.quantity <= 0 || order.price <= 0) {
            return OrderResponse{order.orderId, OrderStatus::REJECTED, 
                                "Invalid order parameters", 0, 0};
        }
        
        // Check if instrument exists
        if (instruments_.find(order.symbol) == instruments_.end()) {
            return OrderResponse{order.orderId, OrderStatus::REJECTED, 
                                "Unknown instrument", 0, 0};
        }
        
        // Add to queue for processing
        order.timestamp = clock_.now();
        orderQueue_.push(std::move(order));
        
        // For immediate response, we'd typically use a promise/future pattern
        // but for high throughput, we use async responses via queue
        return OrderResponse{order.orderId, OrderStatus::PENDING, 
                            "Order accepted", 0, 0};
        
    } catch (const std::exception& e) {
        LOG_ERROR("Error submitting order {}: {}", order.orderId, e.what());
        return OrderResponse{order.orderId, OrderStatus::REJECTED, 
                            "Internal error", 0, 0};
    }
}

void MatchingEngine::processOrders() {
    while (running_.load()) {
        auto order = orderQueue_.pop();
        if (!order) {
            std::this_thread::yield();
            continue;
        }
        
        const auto startTime = clock_.now();
        OrderResponse response;
        
        try {
            processSingleOrder(*order);
            // Response would be generated during processing
        } catch (const std::exception& e) {
            LOG_ERROR("Error processing order {}: {}", order->orderId, e.what());
            response = OrderResponse{order->orderId, OrderStatus::REJECTED, 
                                    "Processing error", 0, 0};
        }
        
        const auto processingTime = clock_.now() - startTime;
        updateMetrics(*order, response, processingTime);
    }
}

void MatchingEngine::processSingleOrder(Order order) {
    // Risk checks
    auto riskCheck = riskEngine_->checkOrder(order);
    if (!riskCheck.approved) {
        sendResponse(OrderResponse{order.orderId, OrderStatus::REJECTED, 
                                  riskCheck.reason, 0, 0});
        return;
    }
    
    // Get instrument order book
    auto& instrument = instruments_.at(order.symbol);
    
    // Process based on order type
    std::vector<Trade> trades;
    {
        std::unique_lock lock(instrument.mutex);
        
        if (order.type == OrderType::LIMIT) {
            trades = instrument.orderBook.addOrder(order);
        } else if (order.type == OrderType::MARKET) {
            trades = instrument.orderBook.matchMarketOrder(order);
        } else if (order.type == OrderType::FOK) {
            trades = instrument.orderBook.matchFOKOrder(order);
        } else if (order.type == OrderType::IOC) {
            trades = instrument.orderBook.matchIOCOrder(order);
        }
        
        // Add to recent trades
        for (const auto& trade : trades) {
            instrument.recentTrades.push_back(trade);
            if (instrument.recentTrades.size() > 1000) {
                instrument.recentTrades.erase(instrument.recentTrades.begin());
            }
            
            // Persist trade
            persistTrade(trade);
            
            // Update risk positions
            riskEngine_->recordTrade(trade);
        }
    }
    
    // Persist order
    persistOrder(order);
    
    // Send response
    OrderResponse response;
    if (!trades.empty()) {
        const auto filled = std::accumulate(trades.begin(), trades.end(), 0, 
            [](Quantity acc, const Trade& trade) { return acc + trade.quantity; });
        
        response = OrderResponse{order.orderId, 
                                filled == order.quantity ? OrderStatus::FILLED : OrderStatus::PARTIAL,
                                "", filled, trades.back().price};
    } else {
        response = OrderResponse{order.orderId, OrderStatus::NEW, "", 0, 0};
    }
    
    sendResponse(response);
    
    // Update metrics
    metrics_->recordOrder(order, response);
}

// Additional implementation for other methods...