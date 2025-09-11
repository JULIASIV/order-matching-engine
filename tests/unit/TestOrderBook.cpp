// tests/unit/TestOrderBook.cpp
#include <gtest/gtest.h>
#include <engine/OrderBook.hpp>
#include <engine/Order.hpp>

class OrderBookTest : public ::testing::Test {
protected:
    void SetUp() override {
        orderBook = std::make_unique<engine::OrderBook>("AAPL");
    }
    
    void TearDown() override {
        orderBook.reset();
    }
    
    std::unique_ptr<engine::OrderBook> orderBook;
};

TEST_F(OrderBookTest, AddBuyOrder) {
    auto order = std::make_shared<engine::Order>(
        1, 100, "AAPL", engine::OrderType::LIMIT, engine::OrderSide::BUY, 
        100.0, 100
    );
    
    auto trades = orderBook->addOrder(order);
    
    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(orderBook->getBestBid(), 100.0);
    EXPECT_TRUE(std::isnan(orderBook->getBestAsk()));
}

TEST_F(OrderBookTest, AddSellOrder) {
    auto order = std::make_shared<engine::Order>(
        1, 100, "AAPL", engine::OrderType::LIMIT, engine::OrderSide::SELL, 
        101.0, 100
    );
    
    auto trades = orderBook->addOrder(order);
    
    EXPECT_TRUE(trades.empty());
    EXPECT_TRUE(std::isnan(orderBook->getBestBid()));
    EXPECT_EQ(orderBook->getBestAsk(), 101.0);
}

TEST_F(OrderBookTest, MatchingOrders) {
    // Add sell order
    auto sellOrder = std::make_shared<engine::Order>(
        1, 100, "AAPL", engine::OrderType::LIMIT, engine::OrderSide::SELL, 
        100.0, 100
    );
    orderBook->addOrder(sellOrder);
    
    // Add buy order that should match
    auto buyOrder = std::make_shared<engine::Order>(
        2, 101, "AAPL", engine::OrderType::LIMIT, engine::OrderSide::BUY, 
        100.0, 100
    );
    auto trades = orderBook->addOrder(buyOrder);
    
    // Check that a trade was executed
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].getQuantity(), 100);
    EXPECT_EQ(trades[0].getPrice(), 100.0);
    
    // Check that both orders are filled
    EXPECT_TRUE(buyOrder->isFilled());
    EXPECT_TRUE(sellOrder->isFilled());
    
    // Check that order book is empty
    EXPECT_TRUE(std::isnan(orderBook->getBestBid()));
    EXPECT_TRUE(std::isnan(orderBook->getBestAsk()));
}

// More tests...