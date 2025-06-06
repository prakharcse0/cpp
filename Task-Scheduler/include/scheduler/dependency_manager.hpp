#pragma once
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <shared_mutex>
#include <atomic>
#include "task.hpp"

class DependencyManager {
private:
    // Adjacency list: dependency_id -> set of dependent task IDs
    std::unordered_map<TaskId, std::unordered_set<TaskId>> dependents_;
    
    // Track how many dependencies each task has left
    std::unordered_map<TaskId, std::atomic<int>> dependency_count_;
    
    mutable std::shared_mutex mutex_;

public:
    // Non-copyable and non-movable
    DependencyManager() = default;
    DependencyManager(const DependencyManager&) = delete;
    DependencyManager& operator=(const DependencyManager&) = delete;
    DependencyManager(DependencyManager&&) = delete;
    DependencyManager& operator=(DependencyManager&&) = delete;
    
    // Dependency management
    void add_dependency(TaskId dependent, TaskId dependency);
    std::vector<TaskId> mark_completed(TaskId task_id);
    void remove_task(TaskId task_id);
    
    // Queries
    bool has_dependencies(TaskId task_id) const;
    int get_dependency_count(TaskId task_id) const;
    std::vector<TaskId> get_dependents(TaskId task_id) const;
    size_t get_pending_task_count() const;
    
    // Cycle detection
    bool has_circular_dependency(TaskId task_id, const std::vector<TaskId>& new_dependencies) const;
    
    // Cleanup
    void clear();

private:
    // Helper methods for cycle detection
    bool creates_cycle(TaskId start, TaskId target) const;
    bool dfs_cycle_detection(TaskId start, TaskId current, 
                            std::unordered_set<TaskId>& visited,
                            std::unordered_set<TaskId>& rec_stack) const;
};