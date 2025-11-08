// src/utils/ThreadPool.cpp
#include "ThreadPool.hpp"
#include "Logger.hpp"
#include <stdexcept>

namespace utils {

ThreadPool::ThreadPool(size_t numThreads) {
    workers_.reserve(numThreads);
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::start() {
    if (running_.exchange(true)) {
        return;
    }
    
    for (size_t i = 0; i < workers_.capacity(); ++i) {
        workers_.emplace_back(&ThreadPool::workerLoop, this);
    }
    
    LOG_INFO("ThreadPool started with {} threads", workers_.size());
}

void ThreadPool::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    
    condition_.notify_all();
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers_.clear();
    LOG_INFO("ThreadPool stopped");
}

void ThreadPool::workerLoop() {
    while (running_.load()) {
        std::function<void()> task;
        
        {
            std::unique_lock lock(queueMutex_);
            condition_.wait(lock, [this]() { 
                return !tasks_.empty() || !running_.load(); 
            });
            
            if (!running_.load() && tasks_.empty()) {
                return;
            }
            
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        
        try {
            task();
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in thread pool worker: {}", e.what());
        }
    }
}

size_t ThreadPool::getQueueSize() const {
    std::unique_lock lock(queueMutex_);
    return tasks_.size();
}

size_t ThreadPool::getThreadCount() const {
    return workers_.size();
}

} // namespace utils