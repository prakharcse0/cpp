#pragma once
#include <memory>
#include <unordered_map>
#include <atomic>
#include <future>
#include <functional>
#include <mutex>
#include <thread>

#include "thread_pool.hpp"
#include "priority_queue.hpp"
#include "dependency_manager.hpp"
#include "task.hpp"

class TaskScheduler {
private:
    std::unique_ptr<ThreadPool> thread_pool_;
    ThreadSafePriorityQueue ready_queue_;
    DependencyManager dependency_manager_;
    
    std::unordered_map<TaskId, std::shared_ptr<Task>> all_tasks_;
    mutable std::mutex tasks_mutex_;
    
    std::atomic<TaskId> next_task_id_;
    std::atomic<bool> shutdown_requested_;

public:
    // Constructor and destructor
    explicit TaskScheduler(size_t num_threads = std::thread::hardware_concurrency());
    ~TaskScheduler();
    
    // Non-copyable and non-movable
    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;
    TaskScheduler(TaskScheduler&&) = delete;
    TaskScheduler& operator=(TaskScheduler&&) = delete;
    
    // Task submission
    TaskId submit_task(std::function<void()> work, 
                      Priority priority = Priority::NORMAL,
                      const std::vector<TaskId>& dependencies = {});
    
    // Template version for easier use
    template<typename F, typename... Args>
    TaskId submit(F&& f, Args&&... args) {
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        return submit_task(std::move(task));
    }
    
    template<typename F, typename... Args>
    TaskId submit_with_priority(Priority priority, F&& f, Args&&... args) {
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        return submit_task(std::move(task), priority);
    }
    
    template<typename F, typename... Args>
    TaskId submit_with_dependencies(const std::vector<TaskId>& dependencies, F&& f, Args&&... args) {
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        return submit_task(std::move(task), Priority::NORMAL, dependencies);
    }
    
    // Task management
    std::future<void> get_task_future(TaskId task_id);
    bool cancel_task(TaskId task_id);
    TaskState get_task_status(TaskId task_id) const;
    
    // Status queries
    size_t pending_tasks() const;
    size_t ready_tasks() const;
    
    // Control
    void shutdown();
    void wait_for_all();

private:
    // Internal methods
    TaskId generate_task_id();
    void schedule_ready_tasks();
    void execute_task(std::shared_ptr<Task> task);
};