#include "OrderBook.h"
#include <zmq.hpp>
#include <iostream>
#include <thread>
#include <atomic>

// Function to handle incoming orders from ZeroMQ
void handleOrders(OrderBook& orderBook, zmq::socket_t& socket) {
    while (true) {
        zmq::message_t request;
        socket.recv(request);

        std::string message(static_cast<char*>(request.data()), request.size());
        if (message == "exit") {
            break;
        } else if (message == "print") {
            std::string orderBookStr = orderBook.getOrderBook();
            zmq::message_t reply(orderBookStr.size());
            memcpy(reply.data(), orderBookStr.c_str(), orderBookStr.size());
            socket.send(reply);
        } else {
            char type = message[0];
            double price = std::stod(message.substr(2, message.find(" ", 2) - 2));
            int quantity = std::stoi(message.substr(message.find(" ", 2) + 1));

            orderBook.addOrder(type, price, quantity);

            std::string response = "Order added: " + message;
            zmq::message_t reply(response.size());
            memcpy(reply.data(), response.c_str(), response.size());
            socket.send(reply);
        }
    }
}

int main() {
    // Initialize ZeroMQ context and socket
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);
    socket.bind("tcp://*:5555");

    // Initialize the order book
    OrderBook orderBook;

    // Start the order matching thread
    std::thread matchingThread(&OrderBook::matchOrders, &orderBook);

    // Handle incoming orders
    handleOrders(orderBook, socket);

    // Clean up
    matchingThread.join();
    return 0;
}
