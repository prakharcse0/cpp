#include "scheduler/dependency_manager.hpp"
#include <algorithm>

void DependencyManager::add_dependency(TaskId dependent, TaskId dependency) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // Add to dependents map
    dependents_[dependency].insert(dependent);
    
    // Increment dependency count for the dependent task
    dependency_count_[dependent]++;
}

std::vector<TaskId> DependencyManager::mark_completed(TaskId task_id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    std::vector<TaskId> ready_tasks;
    
    // Find all tasks that depend on this completed task
    auto it = dependents_.find(task_id);
    if (it != dependents_.end()) {
        for (TaskId dependent_id : it->second) {
            // Decrement dependency count
            auto count_it = dependency_count_.find(dependent_id);
            if (count_it != dependency_count_.end()) {
                int remaining = --count_it->second;
                
                // If no more dependencies, task is ready
                if (remaining == 0) {
                    ready_tasks.push_back(dependent_id);
                    dependency_count_.erase(count_it);
                }
            }
        }
        
        // Clean up completed task from dependents map
        dependents_.erase(it);
    }
    
    return ready_tasks;
}

bool DependencyManager::has_dependencies(TaskId task_id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = dependency_count_.find(task_id);
    return it != dependency_count_.end() && it->second.load() > 0;
}

int DependencyManager::get_dependency_count(TaskId task_id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = dependency_count_.find(task_id);
    return (it != dependency_count_.end()) ? it->second.load() : 0;
}

std::vector<TaskId> DependencyManager::get_dependents(TaskId task_id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = dependents_.find(task_id);
    if (it != dependents_.end()) {
        return std::vector<TaskId>(it->second.begin(), it->second.end());
    }
    
    return {};
}

bool DependencyManager::has_circular_dependency(
    TaskId task_id, 
    const std::vector<TaskId>& new_dependencies) const 
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    // For each new dependency, check if it would create a cycle
    for (TaskId dep : new_dependencies) {
        if (creates_cycle(task_id, dep)) {
            return true;
        }
    }
    
    return false;
}

void DependencyManager::remove_task(TaskId task_id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    // Remove from dependency count
    dependency_count_.erase(task_id);
    
    // Remove from dependents map
    dependents_.erase(task_id);
    
    // Remove this task from other tasks' dependent lists
    for (auto& pair : dependents_) {
        pair.second.erase(task_id);
    }
}

size_t DependencyManager::get_pending_task_count() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return dependency_count_.size();
}

void DependencyManager::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    dependents_.clear();
    dependency_count_.clear();
}

bool DependencyManager::creates_cycle(TaskId start, TaskId target) const {
    // Use DFS to detect if adding target as dependency of start creates cycle
    std::unordered_set<TaskId> visited;
    std::unordered_set<TaskId> rec_stack;
    
    return dfs_cycle_detection(start, target, visited, rec_stack);
}

bool DependencyManager::dfs_cycle_detection(
    TaskId start, 
    TaskId current, 
    std::unordered_set<TaskId>& visited,
    std::unordered_set<TaskId>& rec_stack) const 
{
    // If we've reached the start node again, we have a cycle
    if (current == start && !rec_stack.empty()) {
        return true;
    }
    
    // If already visited in this path, no cycle through this branch
    if (rec_stack.find(current) != rec_stack.end()) {
        return false;
    }
    
    // If already fully explored, no cycle
    if (visited.find(current) != visited.end()) {
        return false;
    }
    
    // Mark as visited and in recursion stack
    visited.insert(current);
    rec_stack.insert(current);
    
    // Check all dependents of current task
    auto it = dependents_.find(current);
    if (it != dependents_.end()) {
        for (TaskId dependent : it->second) {
            if (dfs_cycle_detection(start, dependent, visited, rec_stack)) {
                return true;
            }
        }
    }
    
    // Remove from recursion stack
    rec_stack.erase(current);
    return false;
}