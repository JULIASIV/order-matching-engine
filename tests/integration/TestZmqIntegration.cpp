// tests/integration/TestZmqIntegration.cpp
#include <gtest/gtest.h>
#include <engine/MatchingEngine.hpp>
#include <networking/ZmqInterface.hpp>
#include <thread>
#include <chrono>

class ZmqIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start ZeroMQ interface
        zmqInterface = std::make_unique<networking::ZmqInterface>(
            "tcp://*:5555", "tcp://*:5556"
        );
        zmqInterface->start();
        
        // Start matching engine
        engine = std::make_unique<engine::MatchingEngine>("config/test_config.yaml");
        engine->start();
        
        // Wait for components to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void TearDown() override {
        engine->stop();
        zmqInterface->stop();
    }
    
    std::unique_ptr<networking::ZmqInterface> zmqInterface;
    std::unique_ptr<engine::MatchingEngine> engine;
};

TEST_F(ZmqIntegrationTest, OrderSubmission) {
    // Test order submission via ZeroMQ
    // This would require a client implementation to send and receive messages
    // For now, we'll just verify the components start correctly
    EXPECT_TRUE(engine->getStatus() == engine::EngineStatus::RUNNING);
}

// More integration tests...