// src/networking/ZmqInterface.cpp
#include "ZmqInterface.hpp"
#include "../utils/Logger.hpp"
#include <chrono>
#include <thread>

namespace networking {

ZmqInterface::ZmqInterface(const std::string& publishEndpoint, 
                         const std::string& subscribeEndpoint,
                         size_t queueSize)
    : context_(1)
    , publisher_(context_, ZMQ_PUB)
    , subscriber_(context_, ZMQ_SUB)
    , publishEndpoint_(publishEndpoint)
    , subscribeEndpoint_(subscribeEndpoint)
{
    try {
        publisher_.bind(publishEndpoint_);
        subscriber_.bind(subscribeEndpoint_);
        subscriber_.setsockopt(ZMQ_SUBSCRIBE, "", 0);
        
        LOG_INFO("ZeroMQ interface initialized: pub={}, sub={}", 
                publishEndpoint_, subscribeEndpoint_);
    } catch (const zmq::error_t& e) {
        LOG_ERROR("Failed to initialize ZeroMQ: {}", e.what());
        throw;
    }
}

ZmqInterface::~ZmqInterface() {
    stop();
}

void ZmqInterface::start() {
    if (running_.exchange(true)) {
        return;
    }
    
    publisherThread_ = std::thread(&ZmqInterface::runPublisher, this);
    subscriberThread_ = std::thread(&ZmqInterface::runSubscriber, this);
    
    LOG_INFO("ZeroMQ interface started");
}

void ZmqInterface::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    if (publisherThread_.joinable()) {
        publisherThread_.join();
    }
    
    if (subscriberThread_.joinable()) {
        subscriberThread_.join();
    }
    
    LOG_INFO("ZeroMQ interface stopped");
}

void ZmqInterface::runPublisher() {
    while (running_.load()) {
        auto message = publishQueue_.pop();
        if (!message) {
            std::this_thread::yield();
            continue;
        }
        
        try {
            auto [topic, data] = *message;
            zmq::message_t topicMsg(topic.size());
            memcpy(topicMsg.data(), topic.data(), topic.size());
            
            zmq::message_t dataMsg(data.size());
            memcpy(dataMsg.data(), data.data(), data.size());
            
            publisher_.send(topicMsg, ZMQ_SNDMORE);
            publisher_.send(dataMsg);
        } catch (const zmq::error_t& e) {
            LOG_ERROR("Failed to publish message: {}", e.what());
        }
    }
}

void ZmqInterface::runSubscriber() {
    zmq::pollitem_t items[] = {
        {static_cast<void*>(subscriber_), 0, ZMQ_POLLIN, 0}
    };
    
    while (running_.load()) {
        try {
            zmq::poll(items, 1, 100); // 100ms timeout
            
            if (items[0].revents & ZMQ_POLLIN) {
                zmq::message_t topicMsg;
                zmq::message_t dataMsg;
                
                if (subscriber_.recv(&topicMsg) && subscriber_.recv(&dataMsg)) {
                    std::string topic(static_cast<char*>(topicMsg.data()), topicMsg.size());
                    std::string data(static_cast<char*>(dataMsg.data()), dataMsg.size());
                    
                    std::shared_lock lock(subscriptionsMutex_);
                    if (auto it = subscriptions_.find(topic); it != subscriptions_.end()) {
                        it->second(topic, data);
                    }
                }
            }
        } catch (const zmq::error_t& e) {
            LOG_ERROR("Failed to receive message: {}", e.what());
        }
    }
}

bool ZmqInterface::publish(const std::string& topic, const std::string& message) {
    return publishQueue_.push({topic, message});
}

void ZmqInterface::subscribe(const std::string& topic, MessageCallback callback) {
    std::unique_lock lock(subscriptionsMutex_);
    subscriptions_[topic] = std::move(callback);
}

void ZmqInterface::unsubscribe(const std::string& topic) {
    std::unique_lock lock(subscriptionsMutex_);
    subscriptions_.erase(topic);
}

} // namespace networking