#include "scheduler/priority_queue.hpp"
#include <chrono>

// Task comparator - higher priority tasks come first
bool ThreadSafePriorityQueue::TaskComparator::operator()(
    const std::shared_ptr<Task>& a, 
    const std::shared_ptr<Task>& b) const 
{
    // First compare by priority (higher priority = lower enum value)
    if (a->get_priority() != b->get_priority()) {
        return static_cast<int>(a->get_priority()) < static_cast<int>(b->get_priority());
    }
    
    // If same priority, compare by task ID (FIFO for same priority)
    return a->get_id() > b->get_id();
}

void ThreadSafePriorityQueue::push(std::shared_ptr<Task> task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(task));
    }
    condition_.notify_one();
}

std::shared_ptr<Task> ThreadSafePriorityQueue::pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait until queue is not empty
    condition_.wait(lock, [this] {
        return !queue_.empty();
    });
    
    auto task = queue_.top();
    queue_.pop();
    return task;
}

bool ThreadSafePriorityQueue::try_pop(std::shared_ptr<Task>& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (queue_.empty()) {
        return false;
    }
    
    task = queue_.top();
    queue_.pop();
    return true;
}

std::shared_ptr<Task> ThreadSafePriorityQueue::try_pop_for(
    const std::chrono::milliseconds& timeout) 
{
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (condition_.wait_for(lock, timeout, [this] { return !queue_.empty(); })) {
        auto task = queue_.top();
        queue_.pop();
        return task;
    }
    
    return nullptr;
}

bool ThreadSafePriorityQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

size_t ThreadSafePriorityQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void ThreadSafePriorityQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Clear the queue by creating a new empty one
    std::priority_queue<std::shared_ptr<Task>, 
                       std::vector<std::shared_ptr<Task>>, 
                       TaskComparator> empty_queue;
    queue_.swap(empty_queue);
}

std::vector<std::shared_ptr<Task>> ThreadSafePriorityQueue::drain() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<Task>> tasks;
    
    while (!queue_.empty()) {
        tasks.push_back(queue_.top());
        queue_.pop();
    }
    
    return tasks;
}