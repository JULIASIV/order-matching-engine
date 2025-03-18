#include <iostream>
#include <zmq.hpp>

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REQ);
    socket.connect("tcp://localhost:5555");

    while (true) {
        std::cout << "\n1. Place Buy Order\n2. Place Sell Order\n3. Print Order Book\n4. Exit\nChoice: ";
        int choice;
        std::cin >> choice;

        if (choice == 4) {
            std::string exitMsg = "exit";
            zmq::message_t request(exitMsg.size());
            memcpy(request.data(), exitMsg.c_str(), exitMsg.size());
            socket.send(request);
            break;
        }

        std::string requestStr;
        if (choice == 3) {
            requestStr = "print";
        } else {
            char type = (choice == 1) ? 'B' : 'S';
            double price;
            int quantity;
            std::cout << "Enter price: ";
            std::cin >> price;
            std::cout << "Enter quantity: ";
            std::cin >> quantity;
            requestStr = std::string(1, type) + " " + std::to_string(price) + " " + std::to_string(quantity);
        }

        zmq::message_t request(requestStr.size());
        memcpy(request.data(), requestStr.c_str(), requestStr.size());
        socket.send(request);

        zmq::message_t reply;
        socket.recv(reply);
        std::string replyStr(static_cast<char*>(reply.data()), reply.size());
        std::cout << "Server Response: " << replyStr << std::endl;
    }

    return 0;
}

