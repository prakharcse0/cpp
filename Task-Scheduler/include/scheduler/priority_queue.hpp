#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <chrono>
#include <vector>
#include "task.hpp"

class ThreadSafePriorityQueue {
private:
    struct TaskComparator {
        bool operator()(const std::shared_ptr<Task>& a, 
                       const std::shared_ptr<Task>& b) const;
    };
    
    std::priority_queue<std::shared_ptr<Task>, 
                       std::vector<std::shared_ptr<Task>>, 
                       TaskComparator> queue_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;

public:
    // Non-copyable and non-movable
    ThreadSafePriorityQueue() = default;
    ThreadSafePriorityQueue(const ThreadSafePriorityQueue&) = delete;
    ThreadSafePriorityQueue& operator=(const ThreadSafePriorityQueue&) = delete;
    ThreadSafePriorityQueue(ThreadSafePriorityQueue&&) = delete;
    ThreadSafePriorityQueue& operator=(ThreadSafePriorityQueue&&) = delete;
    
    // Core operations
    void push(std::shared_ptr<Task> task);
    std::shared_ptr<Task> pop();
    bool try_pop(std::shared_ptr<Task>& task);
    std::shared_ptr<Task> try_pop_for(const std::chrono::milliseconds& timeout);
    
    // Status queries
    bool empty() const;
    size_t size() const;
    
    // Bulk operations
    void clear();
    std::vector<std::shared_ptr<Task>> drain();
};