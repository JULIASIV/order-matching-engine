// include/utils/ThreadPool.hpp
#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>

namespace utils {

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();
    
    // Disallow copying
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    
    void start();
    void stop();
    
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using ReturnType = decltype(f(args...));
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            [f = std::forward<F>(f), args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                return std::apply(f, args);
            }
        );
        
        std::future<ReturnType> result = task->get_future();
        
        {
            std::unique_lock lock(queueMutex_);
            if (!running_.load()) {
                throw std::runtime_error("ThreadPool is not running");
            }
            tasks_.emplace([task](){ (*task)(); });
        }
        
        condition_.notify_one();
        return result;
    }
    
    size_t getQueueSize() const;
    size_t getThreadCount() const;
    
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    
    mutable std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> running_{false};
    
    void workerLoop();
};

} // namespace utils