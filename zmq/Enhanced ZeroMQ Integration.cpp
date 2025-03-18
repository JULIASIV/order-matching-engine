void zmqServer(OrderBook& ob) {
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, ZMQ_SUB);
    subscriber.bind("tcp://*:5555");
    subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0);  // Subscribe to all messages

    zmq::socket_t publisher(context, ZMQ_PUB);
    publisher.bind("tcp://*:5556");

    while (true) {
        zmq::message_t request;
        subscriber.recv(request);

        std::string message(static_cast<char*>(request.data()), request.size());
        if (message == "exit") break;
        
        if (message == "print") {
            std::string book = ob.getOrderBook();
            zmq::message_t reply(book.size());
            memcpy(reply.data(), book.c_str(), book.size());
            publisher.send(reply);
            continue;
        }

        // Add order handling logic here

        std::string response = "Order Received\n";
        zmq::message_t reply(response.size());
        memcpy(reply.data(), response.c_str(), response.size());
        publisher.send(reply);
    }
}

