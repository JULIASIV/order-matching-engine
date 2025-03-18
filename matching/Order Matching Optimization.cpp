#include <unordered_map>
#include <vector>
#include <mutex>
#include <iostream>
#include <atomic>
#include <thread>
#include <zmq.hpp>

struct Order {
    int id;
    char type;  // 'B' for buy, 'S' for sell
    double price;
    int quantity;

    Order(int id, char type, double price, int quantity)
        : id(id), type(type), price(price), quantity(quantity) {}
};

class OrderBook {
private:
    std::unordered_map<double, std::vector<Order>> buyOrders;
    std::unordered_map<double, std::vector<Order>> sellOrders;
    std::atomic<int> orderCounter{0};
    std::atomic<bool> running{true};
    std::mutex orderBookMutex;

public:
    void addOrder(char type, double price, int quantity) {
        int id = ++orderCounter;
        Order newOrder(id, type, price, quantity);
        
        std::lock_guard<std::mutex> lock(orderBookMutex);
        if (type == 'B') {
            buyOrders[price].push_back(newOrder);
        } else {
            sellOrders[price].push_back(newOrder);
        }
    }

    void matchOrders() {
        while (running) {
            std::lock_guard<std::mutex> lock(orderBookMutex);
            // Implement order matching logic (buy and sell order matching)
        }
    }

    std::string getOrderBook() {
        std::lock_guard<std::mutex> lock(orderBookMutex);
        std::string book = "SELL ORDERS:\n";
        for (const auto& [price, orders] : sellOrders) {
            for (const auto& order : orders) {
                book += "ID: " + std::to_string(order.id) + " | " + std::to_string(order.quantity) +
                        " @ $" + std::to_string(price) + "\n";
            }
        }

        book += "\nBUY ORDERS:\n";
        for (auto it = buyOrders.rbegin(); it != buyOrders.rend(); ++it) {
            for (const auto& order : it->second) {
                book += "ID: " + std::to_string(order.id) + " | " + std::to_string(order.quantity) +
                        " @ $" + std::to_string(it->first) + "\n";
            }
        }
        return book;
    }

    void stop() { running = false; }
};

