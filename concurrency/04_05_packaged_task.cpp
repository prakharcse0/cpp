#include <iostream>
#include <future>
#include <thread>
#include <vector>
#include <deque>
#include <mutex>
#include <functional>
#include <chrono>
#include <string>
#include <numeric>
#include <cmath>

/*
=== KEY CONCEPTS SUMMARY ===

1. BASIC USAGE:
   - std::packaged_task<ReturnType(Args...)> wraps any callable
   - get_future() returns std::future<ReturnType> for the result
   - Call operator() with Args... to execute the wrapped function

2. THREAD SAFETY:
   - packaged_task itself is NOT thread-safe
   - Multiple threads shouldn't access same packaged_task simultaneously
   - Future can be safely accessed from different thread

3. MOVE SEMANTICS:
   - packaged_task is move-only (cannot be copied)
   - Must use std::move() when passing to threads or containers

4. EXCEPTION HANDLING:
   - Exceptions thrown by wrapped function are captured
   - Retrieved via future.get() in the calling thread

5. PRACTICAL USES:
   - Thread pools and task queues
   - GUI thread message passing
   - Parallel computation with result collection
   - Deferred execution with result retrieval

6. ADVANTAGES OVER std::async:
   - More control over when/how task is executed
   - Can be stored in containers (move-only)
   - Better for custom threading schemes
   - Explicit separation of task creation and execution

7. COMPARED TO std::promise:
   - packaged_task: wraps existing function/callable
   - promise: manually set value/exception
   - packaged_task is higher-level abstraction
*/


// ============================================================================
// PART 1: BASIC std::packaged_task CONCEPTS
// ============================================================================

// Basic function that we'll wrap with packaged_task
int calculate_sum(int a, int b) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
    return a + b;
}

// Function with different signature
std::string process_data(const std::vector<int>& data, double multiplier) {
    int sum = 0;
    for (int val : data) {
        sum += static_cast<int>(val * multiplier);
    }
    return "Processed sum: " + std::to_string(sum);
}

void demonstrate_basic_packaged_task() {
    std::cout << "\n=== BASIC PACKAGED_TASK DEMO ===\n";
    
    // 1. Create a packaged_task with function signature int(int, int)
    std::packaged_task<int(int, int)> task(calculate_sum);
    
    // 2. Get the future BEFORE invoking the task
    std::future<int> result = task.get_future();
    
    // 3. Execute the task (can be done on same thread or different thread)
    task(10, 20); // This calls calculate_sum(10, 20)
    
    // 4. Get the result from the future
    std::cout << "Result: " << result.get() << std::endl;
}

void demonstrate_threaded_packaged_task() {
    std::cout << "\n=== THREADED PACKAGED_TASK DEMO ===\n";
    
    // Create packaged_task for a more complex function
    std::packaged_task<std::string(const std::vector<int>&, double)> task(process_data);
    
    // Get future before moving task to thread
    std::future<std::string> result = task.get_future();
    
    // Move task to a separate thread
    std::vector<int> data = {1, 2, 3, 4, 5};
    std::thread worker(std::move(task), std::cref(data), 2.5);
    
    // Do other work while task runs in background
    std::cout << "Task running in background...\n";
    
    // Get result when ready
    std::cout << "Result: " << result.get() << std::endl;
    
    worker.join();
}

// ============================================================================
// PART 2: LAMBDA FUNCTIONS WITH PACKAGED_TASK
// ============================================================================

void demonstrate_lambda_packaged_task() {
    std::cout << "\n=== LAMBDA PACKAGED_TASK DEMO ===\n";
    
    // Using lambda with packaged_task
    auto lambda_task = [](int x, int y) -> double {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return std::sqrt(x * x + y * y); // Calculate hypotenuse
    };
    
    // Wrap lambda in packaged_task
    std::packaged_task<double(int, int)> task(lambda_task);
    std::future<double> result = task.get_future();
    
    // Execute on separate thread
    std::thread t(std::move(task), 3, 4);
    
    std::cout << "Hypotenuse of (3,4): " << result.get() << std::endl;
    t.join();
}

// ============================================================================
// PART 3: ADVANCED - TASK QUEUE SYSTEM (GUI Thread Example)
// ============================================================================

class TaskQueue {
private:
    std::mutex mutex_;
    std::deque<std::packaged_task<void()>> tasks_;
    bool shutdown_ = false;
    
public:
    // Add a task to the queue and return a future
    template<typename Func>
    std::future<void> post_task(Func&& f) {
        // Create packaged_task from the function
        std::packaged_task<void()> task(std::forward<Func>(f));
        
        // Get future before moving task
        std::future<void> result = task.get_future();
        
        // Add task to queue (thread-safe)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!shutdown_) {
                tasks_.push_back(std::move(task));
            }
        }
        
        return result;
    }
    
    // Process tasks in queue (typically called by worker thread)
    void process_tasks() {
        while (!shutdown_) {
            std::packaged_task<void()> task;
            
            // Get next task from queue
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (tasks_.empty()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                
                task = std::move(tasks_.front());
                tasks_.pop_front();
            }
            
            // Execute task outside of lock
            task();
        }
    }
    
    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        shutdown_ = true;
    }
};

void demonstrate_task_queue() {
    std::cout << "\n=== TASK QUEUE DEMO ===\n";
    
    TaskQueue queue;
    
    // Start worker thread to process tasks
    std::thread worker([&queue]() {
        queue.process_tasks();
    });
    
    // Submit various tasks and collect their futures
    std::vector<std::future<void>> futures;
    
    // Task 1: Simple print
    futures.push_back(queue.post_task([]() {
        std::cout << "Task 1 executed on thread " << std::this_thread::get_id() << std::endl;
    }));
    
    // Task 2: Some computation
    futures.push_back(queue.post_task([]() {
        int sum = 0;
        for (int i = 0; i < 1000; ++i) sum += i;
        std::cout << "Task 2 computed sum: " << sum << std::endl;
    }));
    
    // Task 3: Another print
    futures.push_back(queue.post_task([]() {
        std::cout << "Task 3 executed last\n";
    }));
    
    // Wait for all tasks to complete
    for (auto& future : futures) {
        future.wait();
    }
    
    std::cout << "All tasks completed!\n";
    
    // Shutdown and cleanup
    queue.shutdown();
    worker.join();
}

// ============================================================================
// PART 4: PRACTICAL EXAMPLE - PARALLEL COMPUTATION
// ============================================================================

// Function to compute partial sum of vector segment
int compute_partial_sum(const std::vector<int>& data, size_t start, size_t end) {
    int sum = 0;
    for (size_t i = start; i < end && i < data.size(); ++i) {
        sum += data[i];
        // Simulate some processing time
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
    return sum;
}

void demonstrate_parallel_computation() {
    std::cout << "\n=== PARALLEL COMPUTATION DEMO ===\n";
    
    // Create large dataset
    std::vector<int> data(10000);
    std::iota(data.begin(), data.end(), 1); // Fill with 1, 2, 3, ..., 10000
    
    const size_t num_threads = 4;
    const size_t chunk_size = data.size() / num_threads;
    
    // Create packaged_tasks for parallel computation
    std::vector<std::packaged_task<int(const std::vector<int>&, size_t, size_t)>> tasks;
    std::vector<std::future<int>> futures;
    std::vector<std::thread> threads;
    
    // Create tasks and futures
    for (size_t i = 0; i < num_threads; ++i) {
        tasks.emplace_back(compute_partial_sum);
        futures.push_back(tasks[i].get_future());
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Launch threads with tasks
    for (size_t i = 0; i < num_threads; ++i) {
        size_t start_idx = i * chunk_size;
        size_t end_idx = (i == num_threads - 1) ? data.size() : (i + 1) * chunk_size;
        
        threads.emplace_back(std::move(tasks[i]), std::cref(data), start_idx, end_idx);
    }
    
    // Collect results
    int total_sum = 0;
    for (auto& future : futures) {
        total_sum += future.get();
    }
    
    // Wait for all threads
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Parallel sum: " << total_sum << std::endl;
    std::cout << "Time taken: " << duration.count() << " ms" << std::endl;
    
    // Verify with sequential computation
    int sequential_sum = std::accumulate(data.begin(), data.end(), 0);
    std::cout << "Sequential sum: " << sequential_sum << std::endl;
    std::cout << "Results match: " << (total_sum == sequential_sum ? "Yes" : "No") << std::endl;
}

// ============================================================================
// PART 5: ERROR HANDLING WITH PACKAGED_TASK
// ============================================================================

int risky_function(int value) {
    if (value < 0) {
        throw std::invalid_argument("Negative values not allowed");
    }
    return value * 2;
}

void demonstrate_exception_handling() {
    std::cout << "\n=== EXCEPTION HANDLING DEMO ===\n";
    
    // Test 1: Normal execution
    {
        std::packaged_task<int(int)> task(risky_function);
        std::future<int> result = task.get_future();
        
        task(5); // Normal case
        
        try {
            std::cout << "Normal result: " << result.get() << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Exception caught: " << e.what() << std::endl;
        }
    }
    
    // Test 2: Exception case
    {
        std::packaged_task<int(int)> task(risky_function);
        std::future<int> result = task.get_future();
        
        task(-5); // This will throw
        
        try {
            std::cout << "Result: " << result.get() << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Exception caught: " << e.what() << std::endl;
        }
    }
}

// ============================================================================
// PART 6: COMPARISON WITH OTHER ASYNC MECHANISMS
// ============================================================================

void demonstrate_comparison() {
    std::cout << "\n=== COMPARISON WITH OTHER MECHANISMS ===\n";
    
    auto work_function = [](int x) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return x * x;
    };
    
    // Method 1: std::async
    std::cout << "Using std::async:\n";
    auto async_future = std::async(std::launch::async, work_function, 5);
    std::cout << "Result: " << async_future.get() << std::endl;
    
    // Method 2: std::packaged_task
    std::cout << "Using std::packaged_task:\n";
    std::packaged_task<int(int)> task(work_function);
    auto task_future = task.get_future();
    std::thread t(std::move(task), 5);
    std::cout << "Result: " << task_future.get() << std::endl;
    t.join();
    
    // Method 3: std::promise (for comparison)
    std::cout << "Using std::promise:\n";
    std::promise<int> promise;
    auto promise_future = promise.get_future();
    std::thread t2([&promise, work_function]() {
        try {
            int result = work_function(5);
            promise.set_value(result);
        } catch (...) {
            promise.set_exception(std::current_exception());
        }
    });
    std::cout << "Result: " << promise_future.get() << std::endl;
    t2.join();
}

// ============================================================================
// MAIN FUNCTION - RUN ALL DEMONSTRATIONS
// ============================================================================

int main() {
    std::cout << "=== COMPREHENSIVE std::packaged_task TUTORIAL ===\n";
    std::cout << "Main thread ID: " << std::this_thread::get_id() << std::endl;
    
    try {
        demonstrate_basic_packaged_task();
        demonstrate_threaded_packaged_task();
        demonstrate_lambda_packaged_task();
        demonstrate_task_queue();
        demonstrate_parallel_computation();
        demonstrate_exception_handling();
        demonstrate_comparison();
        
        std::cout << "\n=== TUTORIAL COMPLETED SUCCESSFULLY ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
