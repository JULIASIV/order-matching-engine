// src/engine/main.cpp (Enhanced)
#include "MatchingEngine.hpp"
#include "../networking/ZmqInterface.hpp"
#include "../networking/FixAdapter.hpp"
#include "../api/RestApi.hpp"
#include "../risk/CircuitBreaker.hpp"
#include "../feeds/WebSocketFeed.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Config.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <memory>

std::atomic<bool> running{true};

void signalHandler(int signal) {
    LOG_INFO("Received signal {}, shutting down", signal);
    running = false;
}

int main(int argc, char* argv[]) {
    try {
        // Setup signal handling
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);
        
        // Load configuration
        utils::Config config("config/config.yaml");
        
        // Initialize logger
        utils::Logger::init(config.get<std::string>("logging.level", "info"),
                           config.get<std::string>("logging.file", ""));
        
        LOG_INFO("Starting World-Class Order Matching Engine");
        
        // Initialize core components
        auto matchingEngine = std::make_shared<engine::MatchingEngine>(config);
        auto zmqInterface = std::make_shared<networking::ZmqInterface>(
            config.get<std::string>("network.publish_endpoint", "tcp://*:5555"),
            config.get<std::string>("network.subscribe_endpoint", "tcp://*:5556")
        );
        
        auto riskEngine = std::make_shared<risk::RiskEngine>(config);
        auto circuitBreaker = std::make_shared<risk::CircuitBreaker>(config);
        auto metrics = std::make_shared<monitoring::Metrics>(config);
        
        // Initialize FIX adapter if configured
        std::unique_ptr<networking::FixAdapter> fixAdapter;
        if (config.has("fix.enabled") && config.get<bool>("fix.enabled")) {
            fixAdapter = std::make_unique<networking::FixAdapter>(
                matchingEngine, 
                config.get<std::string>("fix.config_file", "config/fix.cfg")
            );
        }
        
        // Initialize REST API
        auto restApi = std::make_shared<api::RestApi>(
            config.get<std::string>("api.address", "http://0.0.0.0:8080"),
            matchingEngine,
            metrics,
            riskEngine
        );
        
        // Initialize market data feed if configured
        std::unique_ptr<feeds::WebSocketFeed> marketDataFeed;
        if (config.has("market_data.enabled") && config.get<bool>("market_data.enabled")) {
            // This would be implemented based on the specific market data provider
            // marketDataFeed = std::make_unique<feeds::WebSocketFeed>(...);
        }
        
        // Start components
        LOG_INFO("Starting core components...");
        matchingEngine->start();
        zmqInterface->start();
        metrics->startExposer(config.get<std::string>("monitoring.endpoint", "0.0.0.0:9090"));
        restApi->start();
        
        if (fixAdapter) {
            fixAdapter->start();
        }
        
        if (marketDataFeed) {
            marketDataFeed->start();
        }
        
        LOG_INFO("Order Matching Engine started successfully");
        LOG_INFO("REST API: {}", config.get<std::string>("api.address", "http://0.0.0.0:8080"));
        LOG_INFO("Metrics: {}", config.get<std::string>("monitoring.endpoint", "0.0.0.0:9090"));
        
        // Main loop
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Print statistics periodically
            static auto lastStatsTime = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            
            if (now - lastStatsTime > std::chrono::seconds(10)) {
                auto stats = matchingEngine->getStatistics();
                auto status = matchingEngine->getStatus();
                
                LOG_INFO("Engine Status: {}, Orders: {}, Trades: {}, Avg Latency: {}ns", 
                        static_cast<int>(status), stats.ordersProcessed, 
                        stats.tradesExecuted, stats.avgLatencyNs);
                
                // Check circuit breaker status
                // This would check if any symbols are halted
                
                lastStatsTime = now;
            }
            
            // Health check - restart components if needed
            // This would monitor component health and restart if necessary
        }
        
        // Graceful shutdown
        LOG_INFO("Initiating graceful shutdown...");
        
        if (marketDataFeed) {
            marketDataFeed->stop();
        }
        
        if (fixAdapter) {
            fixAdapter->stop();
        }
        
        restApi->stop();
        zmqInterface->stop();
        matchingEngine->stop();
        
        LOG_INFO("Order Matching Engine stopped successfully");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Fatal error: {}", e.what());
        return 1;
    }
    
    return 0;
}