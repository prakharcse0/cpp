#include "scheduler/task_scheduler.hpp"
#include <algorithm>
#include <iostream>

TaskScheduler::TaskScheduler(size_t num_threads)
    : thread_pool_(std::make_unique<ThreadPool>(num_threads))
    , next_task_id_(1)
    , shutdown_requested_(false)
{
}

TaskScheduler::~TaskScheduler() {
    shutdown();
}

TaskId TaskScheduler::submit_task(
    std::function<void()> work, 
    Priority priority,
    const std::vector<TaskId>& dependencies) 
{
    if (shutdown_requested_.load()) {
        throw std::runtime_error("Cannot submit task: scheduler is shutting down");
    }
    
    TaskId task_id = generate_task_id();
    
    // Check for circular dependencies before creating task
    if (!dependencies.empty()) {
        if (dependency_manager_.has_circular_dependency(task_id, dependencies)) {
            throw std::runtime_error("Circular dependency detected");
        }
    }
    
    // Create the task
    auto task = std::make_shared<Task>(task_id, std::move(work), priority);
    
    {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        all_tasks_[task_id] = task;
    }
    
    // Add dependencies
    for (TaskId dep : dependencies) {
        // Verify dependency task exists
        {
            std::lock_guard<std::mutex> lock(tasks_mutex_);
            if (all_tasks_.find(dep) == all_tasks_.end()) {
                throw std::runtime_error("Dependency task does not exist: " + std::to_string(dep));
            }
        }
        
        task->add_dependency(dep);
        dependency_manager_.add_dependency(task_id, dep);
    }
    
    // If no dependencies, task is ready to run
    if (dependencies.empty()) {
        task->set_state(TaskState::READY);
        ready_queue_.push(task);
    }
    
    // Start processing ready tasks
    schedule_ready_tasks();
    
    return task_id;
}

std::future<void> TaskScheduler::get_task_future(TaskId task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    auto it = all_tasks_.find(task_id);
    if (it == all_tasks_.end()) {
        throw std::runtime_error("Task not found: " + std::to_string(task_id));
    }
    
    return it->second->get_future();
}

bool TaskScheduler::cancel_task(TaskId task_id) {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    auto it = all_tasks_.find(task_id);
    if (it == all_tasks_.end()) {
        return false;  // Task doesn't exist
    }
    
    auto& task = it->second;
    TaskState current_state = task->get_state();
    
    // Can only cancel pending or ready tasks
    if (current_state == TaskState::PENDING || current_state == TaskState::READY) {
        task->request_cancel();
        task->set_state(TaskState::CANCELLED);
        
        // Remove from dependency manager
        dependency_manager_.remove_task(task_id);
        
        return true;
    }
    
    // For running tasks, request cancellation (cooperative)
    if (current_state == TaskState::RUNNING) {
        task->request_cancel();
        return true;
    }
    
    return false;  // Already completed or cancelled
}

TaskState TaskScheduler::get_task_status(TaskId task_id) const {
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    
    auto it = all_tasks_.find(task_id);
    if (it == all_tasks_.end()) {
        throw std::runtime_error("Task not found: " + std::to_string(task_id));
    }
    
    return it->second->get_state();
}

size_t TaskScheduler::pending_tasks() const {
    return dependency_manager_.get_pending_task_count() + ready_queue_.size();
}

size_t TaskScheduler::ready_tasks() const {
    return ready_queue_.size();
}

void TaskScheduler::shutdown() {
    shutdown_requested_.store(true);
    
    // Cancel all pending tasks
    std::lock_guard<std::mutex> lock(tasks_mutex_);
    for (auto& pair : all_tasks_) {
        auto& task = pair.second;
        if (task->get_state() == TaskState::PENDING || task->get_state() == TaskState::READY) {
            task->request_cancel();
            task->set_state(TaskState::CANCELLED);
        }
    }
    
    // Clear queues
    ready_queue_.clear();
    dependency_manager_.clear();
    
    // Shutdown thread pool
    if (thread_pool_) {
        thread_pool_->shutdown();
    }
}

void TaskScheduler::wait_for_all() {
    while (pending_tasks() > 0 && !shutdown_requested_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

TaskId TaskScheduler::generate_task_id() {
    return next_task_id_.fetch_add(1);
}

void TaskScheduler::schedule_ready_tasks() {
    // This method is called whenever tasks might become ready
    // The actual scheduling happens in the thread pool workers
    // by continuously polling the ready_queue_
    
    if (!thread_pool_) return;
    
    // Submit a worker function that processes the ready queue
    thread_pool_->enqueue([this]() {
        std::shared_ptr<Task> task;
        
        // Try to get a task with timeout to avoid blocking forever
        if ((task = ready_queue_.try_pop_for(std::chrono::milliseconds(100)))) {
            execute_task(task);
        }
    });
}

void TaskScheduler::execute_task(std::shared_ptr<Task> task) {
    if (!task || shutdown_requested_.load()) {
        return;
    }
    
    TaskId task_id = task->get_id();
    
    // Execute the task
    task->execute();
    
    // If task completed successfully, check for newly ready tasks
    if (task->get_state() == TaskState::COMPLETED) {
        auto ready_tasks = dependency_manager_.mark_completed(task_id);
        
        // Add newly ready tasks to the queue
        {
            std::lock_guard<std::mutex> lock(tasks_mutex_);
            for (TaskId ready_id : ready_tasks) {
                auto it = all_tasks_.find(ready_id);
                if (it != all_tasks_.end()) {
                    auto& ready_task = it->second;
                    ready_task->set_state(TaskState::READY);
                    ready_queue_.push(ready_task);
                }
            }
        }
        
        // Schedule processing of newly ready tasks
        if (!ready_tasks.empty()) {
            schedule_ready_tasks();
        }
    }
    
    // Clean up completed task after some time (optional)
    // In a production system, you might want to keep completed tasks
    // for a while for status queries, then clean them up
}