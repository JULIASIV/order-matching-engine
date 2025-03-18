#include <iostream>
#include <map>
#include <vector>
#include <thread>
#include <atomic>
#include <boost/lockfree/queue.hpp>

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
    std::map<double, std::vector<Order>> buyOrders;  // Max price first
    std::map<double, std::vector<Order>> sellOrders; // Min price first
    std::atomic<int> orderCounter{0};                // Atomic counter for order IDs
    boost::lockfree::queue<Order*> orderQueue{128};  // Lock-free queue for incoming orders
    std::atomic<bool> running{true};                 // Flag to control the matching thread

public:
    void addOrder(char type, double price, int quantity) {
        int id = ++orderCounter;
        Order* newOrder = new Order(id, type, price, quantity);
        while (!orderQueue.push(newOrder)) {}  // Wait until queue has space
    }

    void matchOrders() {
        while (running) {
            Order* incoming;
            if (orderQueue.pop(incoming)) {
                processOrder(*incoming);
                delete incoming;  // Free memory after processing
            }
        }
    }

    void processOrder(Order& newOrder) {
        if (newOrder.type == 'B') {
            matchOrder(newOrder, sellOrders);
            if (newOrder.quantity > 0) {
                buyOrders[newOrder.price].push_back(newOrder);
            }
        } else if (newOrder.type == 'S') {
            matchOrder(newOrder, buyOrders);
            if (newOrder.quantity > 0) {
                sellOrders[newOrder.price].push_back(newOrder);
            }
        }
    }

    void matchOrder(Order& incoming, std::map<double, std::vector<Order>>& oppositeBook) {
        auto it = (incoming.type == 'B') ? oppositeBook.begin() : oppositeBook.rbegin();

        while (incoming.quantity > 0 && it != oppositeBook.end()) {
            if ((incoming.type == 'B' && it->first > incoming.price) ||
                (incoming.type == 'S' && it->first < incoming.price)) {
                break;
            }

            std::vector<Order>& orders = it->second;
            auto iter = orders.begin();
            
            while (iter != orders.end() && incoming.quantity > 0) {
                int tradeQuantity = std::min(incoming.quantity, iter->quantity);
                incoming.quantity -= tradeQuantity;
                iter->quantity -= tradeQuantity;

                std::cout << "Trade Executed: " << tradeQuantity 
                          << " @ $" << it->first << std::endl;

                if (iter->quantity == 0) {
                    iter = orders.erase(iter);
                } else {
                    ++iter;
                }
            }

            if (orders.empty()) {
                it = (incoming.type == 'B') ? oppositeBook.erase(it) : oppositeBook.erase(--it.base());
            } else {
                break;
            }
        }
    }

    void printOrderBook() {
        std::cout << "\nOrder Book:\n";
        std::cout << "SELL ORDERS:\n";
        for (const auto& [price, orders] : sellOrders) {
            for (const auto& order : orders) {
                std::cout << "ID: " << order.id << " | " << order.quantity << " @ $" << price << "\n";
            }
        }

        std::cout << "\nBUY ORDERS:\n";
        for (auto it = buyOrders.rbegin(); it != buyOrders.rend(); ++it) {
            for (const auto& order : it->second) {
                std::cout << "ID: " << order.id << " | " << order.quantity << " @ $" << it->first << "\n";
            }
        }
    }

    void stop() {
        running = false;
    }
};

int main() {
    OrderBook ob;
    std::thread matchingThread(&OrderBook::matchOrders, &ob);

    int choice;
    
    while (true) {
        std::cout << "\n1. Place Buy Order\n2. Place Sell Order\n3. Print Order Book\n4. Exit\nChoice: ";
        std::cin >> choice;

        if (choice == 4) {
            ob.stop();
            break;
        }

        double price;
        int quantity;
        if (choice == 1 || choice == 2) {
            std::cout << "Enter price: ";
            std::cin >> price;
            std::cout << "Enter quantity: ";
            std::cin >> quantity;
            ob.addOrder((choice == 1) ? 'B' : 'S', price, quantity);
        } else if (choice == 3) {
            ob.printOrderBook();
        }
    }

    matchingThread.join();
    return 0;
}

