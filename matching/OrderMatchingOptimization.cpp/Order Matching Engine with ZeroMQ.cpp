#include <iostream>
#include <map>
#include <vector>
#include <thread>
#include <atomic>
#include <boost/lockfree/queue.hpp>
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
    std::map<double, std::vector<Order>> buyOrders;
    std::map<double, std::vector<Order>> sellOrders;
    std::atomic<int> orderCounter{0};
    boost::lockfree::queue<Order*> orderQueue{128};
    std::atomic<bool> running{true};

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
            if (newOrder.quantity > 0) buyOrders[newOrder.price].push_back(newOrder);
        } else {
            matchOrder(newOrder, buyOrders);
            if (newOrder.quantity > 0) sellOrders[newOrder.price].push_back(newOrder);
        }
    }

    void matchOrder(Order& incoming, std::map<double, std::vector<Order>>& oppositeBook) {
        auto it = (incoming.type == 'B') ? oppositeBook.begin() : oppositeBook.rbegin();

        while (incoming.quantity > 0 && it != oppositeBook.end()) {
            if ((incoming.type == 'B' && it->first > incoming.price) ||
                (incoming.type == 'S' && it->first < incoming.price)) break;

            std::vector<Order>& orders = it->second;
            auto iter = orders.begin();
            
            while (iter != orders.end() && incoming.quantity > 0) {
                int tradeQuantity = std::min(incoming.quantity, iter->quantity);
                incoming.quantity -= tradeQuantity;
                iter->quantity -= tradeQuantity;

                std::cout << "Trade Executed: " << tradeQuantity << " @ $" << it->first << std::endl;

                if (iter->quantity == 0) iter = orders.erase(iter);
                else ++iter;
            }

            if (orders.empty()) {
                it = (incoming.type == 'B') ? oppositeBook.erase(it) : oppositeBook.erase(--it.base());
            } else break;
        }
    }

    std::string getOrderBook() {
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

// ZeroMQ Server (Matching Engine)
void zmqServer(OrderBook& ob) {
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);
    socket.bind("tcp://*:5555");

    while (true) {
        zmq::message_t request;
        socket.recv(request);

        std::string input(static_cast<char*>(request.data()), request.size());
        if (input == "exit") break;
        
        if (input == "print") {
            std::string book = ob.getOrderBook();
            zmq::message_t reply(book.size());
            memcpy(reply.data(), book.c_str(), book.size());
            socket.send(reply);
            continue;
        }

        char type;
        double price;
        int quantity;
        sscanf(input.c_str(), "%c %lf %d", &type, &price, &quantity);
        
        ob.addOrder(type, price, quantity);
        std::string response = "Order Received\n";
        zmq::message_t reply(response.size());
        memcpy(reply.data(), response.c_str(), response.size());
        socket.send(reply);
    }
}

int main() {
    OrderBook ob;
    std::thread matchingThread(&OrderBook::matchOrders, &ob);
    std::thread serverThread(zmqServer, std::ref(ob));

    std::cout << "ZeroMQ Server Running on Port 5555...\n";
    serverThread.join();
    
    ob.stop();
    matchingThread.join();
    return 0;
}



