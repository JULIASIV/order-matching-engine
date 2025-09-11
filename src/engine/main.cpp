// src/engine/main.cpp
#include "MatchingEngine.hpp"
#include "../networking/ZmqInterface.hpp"
#include "../utils/Logger.hpp"
#include "../utils/Config.hpp"
#include <iostream>
#include <csignal>
#include <atomic>

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
        
        LOG_INFO("Starting Order Matching Engine");
        
        // Initialize components
        engine::MatchingEngine matchingEngine(config);
        networking::ZmqInterface zmqInterface(
            config.get<std::string>("network.publish_endpoint", "tcp://*:5555"),
            config.get<std::string>("network.subscribe_endpoint", "tcp://*:5556")
        );
        
        // Start components
        matchingEngine.start();
        zmqInterface.start();
        
        LOG_INFO("Order Matching Engine started successfully");
        
        // Main loop
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Print statistics periodically
            static auto lastStatsTime = std::chrono::steady_clock::now();
            auto now = std::chrono::steady_clock::now();
            
            if (now - lastStatsTime > std::chrono::seconds(5)) {
                auto stats = matchingEngine.getStatistics();
                LOG_INFO("Statistics: orders={}, trades={}, avg_latency={}ns", 
                        stats.ordersProcessed, stats.tradesExecuted, stats.avgLatencyNs);
                lastStatsTime = now;
            }
        }
        
        // Shutdown
        LOG_INFO("Shutting down Order Matching Engine");
        matchingEngine.stop();
        zmqInterface.stop();
        
        LOG_INFO("Order Matching Engine stopped successfully");
        
    } catch (const std::exception& e) {
        LOG_ERROR("Fatal error: {}", e.what());
        return 1;
    }
    
    return 0;
}