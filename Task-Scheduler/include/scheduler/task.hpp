#pragma once
#include <functional>
#include <future>
#include <atomic>
#include <chrono>
#include <vector>
#include <memory>

enum class TaskState {
    PENDING,
    READY,
    RUNNING,
    COMPLETED,
    CANCELLED
};

enum class Priority {
    LOW = 0,
    NORMAL = 1,
    HIGH = 2
};

using TaskId = std::size_t;

class Task {
private:
    TaskId id_;
    std::function<void()> work_;
    Priority priority_;
    std::atomic<TaskState> state_;
    std::vector<TaskId> dependencies_;
    std::promise<void> completion_promise_;
    std::atomic<bool> cancel_requested_;

public:
    // Constructor
    Task(TaskId id, std::function<void()> work, Priority priority = Priority::NORMAL);
    
    // Getters
    TaskId get_id() const;
    Priority get_priority() const;
    TaskState get_state() const;
    const std::vector<TaskId>& get_dependencies() const;
    
    // State management
    void set_state(TaskState new_state);
    bool is_ready() const;
    bool is_completed() const;
    
    // Dependency management
    void add_dependency(TaskId dependency);
    
    // Execution
    void execute();
    
    // Cancellation
    void request_cancel();
    bool is_cancel_requested() const;
    
    // Future support
    std::future<void> get_future();
};