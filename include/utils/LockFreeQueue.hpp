// include/utils/LockFreeQueue.hpp
#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <array>
#include <cassert>

namespace utils {

template<typename T, size_t Capacity>
class LockFreeQueue {
private:
    struct Node {
        std::atomic<bool> occupied{false};
        alignas(64) T data;
    };
    
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
    alignas(64) std::array<Node, Capacity> buffer_;
    
public:
    LockFreeQueue() = default;
    ~LockFreeQueue() = default;
    
    // Disallow copying
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;
    
    bool push(T value) {
        size_t current_tail = tail_.load(std::memory_order_relaxed);
        size_t next_tail = (current_tail + 1) % Capacity;
        
        // Check if queue is full
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false;
        }
        
        // Try to claim the slot
        if (!buffer_[current_tail].occupied.load(std::memory_order_acquire)) {
            buffer_[current_tail].data = std::move(value);
            buffer_[current_tail].occupied.store(true, std::memory_order_release);
            tail_.store(next_tail, std::memory_order_release);
            return true;
        }
        
        return false;
    }
    
    std::optional<T> pop() {
        size_t current_head = head_.load(std::memory_order_relaxed);
        
        // Check if queue is empty
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return std::nullopt;
        }
        
        // Try to read the slot
        if (buffer_[current_head].occupied.load(std::memory_order_acquire)) {
            T value = std::move(buffer_[current_head].data);
            buffer_[current_head].occupied.store(false, std::memory_order_release);
            head_.store((current_head + 1) % Capacity, std::memory_order_release);
            return value;
        }
        
        return std::nullopt;
    }
    
    bool empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }
    
    size_t size() const {
        size_t head = head_.load(std::memory_order_acquire);
        size_t tail = tail_.load(std::memory_order_acquire);
        
        if (tail >= head) {
            return tail - head;
        } else {
            return Capacity - head + tail;
        }
    }
    
    static constexpr size_t capacity() {
        return Capacity;
    }
};

} // namespace utils