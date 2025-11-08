// include/networking/ZmqPublisher.hpp
#pragma once

#include <zmq.hpp>
#include <string>
#include <thread>
#include <atomic>
#include "LockFreeQueue.hpp"
#include "MessageTypes.hpp"

namespace networking {

class ZmqPublisher {
public:
    ZmqPublisher(const std::string& endpoint, size_t queue_size = 10000);
    ~ZmqPublisher();
    
    void start();
    void stop();
    
    template<typename MessageType>
    void publish(const MessageType& message) {
        std::string serialized = serializeMessage(message);
        queue_.push(std::move(serialized));
    }
    
private:
    zmq::context_t context_;
    zmq::socket_t socket_;
    std::string endpoint_;
    
    LockFreeQueue<std::string> queue_;
    std::atomic<bool> running_{false};
    std::thread workerThread_;
    
    void run();
    std::string serializeMessage(const OrderBookUpdate& update);
    std::string serializeMessage(const Trade& trade);
    std::string serializeMessage(const OrderAck& ack);
};

} // namespace networking