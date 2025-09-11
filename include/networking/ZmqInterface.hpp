// include/networking/ZmqInterface.hpp
#pragma once

#include <zmq.hpp>
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <shared_mutex>
#include "../utils/LockFreeQueue.hpp"

namespace networking {

class ZmqInterface {
public:
    using MessageCallback = std::function<void(const std::string& topic, const std::string& message)>;
    
    ZmqInterface(const std::string& publishEndpoint, 
                 const std::string& subscribeEndpoint,
                 size_t queueSize = 100000);
    ~ZmqInterface();
    
    void start();
    void stop();
    
    bool publish(const std::string& topic, const std::string& message);
    bool publish(const std::string& topic, const std::vector<char>& data);
    
    void subscribe(const std::string& topic, MessageCallback callback);
    void unsubscribe(const std::string& topic);
    
    // High-throughput batch publishing
    bool publishBatch(const std::vector<std::pair<std::string, std::string>>& messages);
    
private:
    zmq::context_t context_;
    zmq::socket_t publisher_;
    zmq::socket_t subscriber_;
    
    std::string publishEndpoint_;
    std::string subscribeEndpoint_;
    
    utils::LockFreeQueue<std::pair<std::string, std::string>, 100000> publishQueue_;
    std::unordered_map<std::string, MessageCallback> subscriptions_;
    mutable std::shared_mutex subscriptionsMutex_;
    
    std::atomic<bool> running_{false};
    std::thread publisherThread_;
    std::thread subscriberThread_;
    
    void runPublisher();
    void runSubscriber();
    
    // Zero-copy message building
    zmq::message_t buildMessage(const std::string& topic, const std::string& data) const;
};

} // namespace networking