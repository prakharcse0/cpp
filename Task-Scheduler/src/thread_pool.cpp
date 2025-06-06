#include "scheduler/thread_pool.hpp"
#include <iostream>

ThreadPool::ThreadPool(size_t num_threads) 
    : stop_(false)
    , num_threads_(num_threads)
{
    // Create worker threads
    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::worker_loop, this);
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        // Don't accept new tasks if stopping
        if (stop_) {
            return;
        }
        
        task_queue_.push(std::move(task));
    }
    
    // Notify one waiting worker
    condition_.notify_one();
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stop_ = true;
    }
    
    // Wake up all workers
    condition_.notify_all();
    
    // Wait for all workers to finish
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers_.clear();
}

size_t ThreadPool::get_thread_count() const {
    return num_threads_;
}

size_t ThreadPool::get_queue_size() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return task_queue_.size();
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            // Wait for work or stop signal
            condition_.wait(lock, [this] {
                return stop_ || !task_queue_.empty();
            });
            
            // Exit if stopping and no more work
            if (stop_ && task_queue_.empty()) {
                break;
            }
            
            // Get next task
            if (!task_queue_.empty()) {
                task = std::move(task_queue_.front());
                task_queue_.pop();
            }
        }
        
        // Execute task outside of lock
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "Exception in worker thread: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown exception in worker thread" << std::endl;
            }
        }
    }
}