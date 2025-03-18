#include <iostream>
#include <zmq.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <fstream>
#include <thread>

struct Order {
    int id;
    char type;  // 'B' for Buy, 'S' for Sell
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
        while (true) {
            std::lock_guard<std::mutex> lock(orderBookMutex);
            if (!buyOrders.empty() && !sellOrders.empty()) {
                // Try to match the highest buy order with the lowest sell order
                auto buyIt = buyOrders.rbegin();  // Highest price
                auto sellIt = sellOrders.begin(); // Lowest price

                if (buyIt->first >= sellIt->first) {
                    int matchedQuantity = std::min(buyIt->second[0].quantity, sellIt->second[0].quantity);
                    buyIt->second[0].quantity -= matchedQuantity;
                    sellIt->second[0].quantity -= matchedQuantity;

                    std::string matched = "Matched: Buy " + std::to_string(matchedQuantity) + " at $" + std::to_string(sellIt->first);
                    std::cout << matched << std::endl;

                    if (buyIt->second[0].quantity == 0) buyOrders.erase(buyIt->first);
                    if (sellIt->second[0].quantity == 0) sellOrders.erase(sellIt->first);
                }
            }
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
};

void zmqServer() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);  // REP socket for responding to client requests
    socket.bind("tcp://*:5555");

    OrderBook orderBook;
    std::thread matcherThread(&OrderBook::matchOrders, &orderBook);

    while (true) {
        zmq::message_t request;
        socket.recv(request);

        std::string requestStr(static_cast<char*>(request.data()), request.size());
        if (requestStr == "exit") {
            break;
        } else if (requestStr == "print") {
            std::string orderBookStr = orderBook.getOrderBook();
            zmq::message_t reply(orderBookStr.size());
            memcpy(reply.data(), orderBookStr.c_str(), orderBookStr.size());
            socket.send(reply);
        } else {
            char type = requestStr[0];
            double price = std::stod(requestStr.substr(2, requestStr.find(" ", 2) - 2));
            int quantity = std::stoi(requestStr.substr(requestStr.find(" ", 2) + 1));

            orderBook.addOrder(type, price, quantity);

            std::string response = "Order added: " + requestStr;
            zmq::message_t reply(response.size());
            memcpy(reply.data(), response.c_str(), response.size());
            socket.send(reply);
        }
    }

    matcherThread.join();  // Ensure the order matching thread finishes
}

int main() {
    zmqServer();
    return 0;
}

