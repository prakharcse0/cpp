#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <iostream>
#include <thread>

template<typename T>
class threadsafe_queue_optimized {
private:
    mutable std::mutex mut;
    
    // KEY OPTIMIZATION: Store shared_ptrs instead of raw values
    // This moves memory allocation outside the critical section
    std::queue<std::shared_ptr<T>> data_queue;
    
    std::condition_variable data_cond;

public:
    threadsafe_queue_optimized() {}
    
    // Push now does allocation OUTSIDE the lock - major performance improvement
    void push(T new_value) {
        // Allocate memory outside the critical section
        // This reduces lock contention and improves concurrent performance
        std::shared_ptr<T> data(
            std::make_shared<T>(std::move(new_value))
        );
        
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(data);
        data_cond.notify_one();
    }
    
    // Wait and pop with reference - dereferences the stored shared_ptr
    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] { return !data_queue.empty(); });
        
        // Dereference the shared_ptr to get the actual value
        value = std::move(*data_queue.front());
        data_queue.pop();
    }
    
    // Wait and pop returning shared_ptr - just returns the stored pointer
    // No additional allocation needed - very efficient
    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] { return !data_queue.empty(); });
        
        // Simply move the shared_ptr from queue - no allocation
        std::shared_ptr<T> res = data_queue.front();
        data_queue.pop();
        return res;
    }
    
    // Non-blocking pop with reference
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lk(mut);
        
        if(data_queue.empty())
            return false;
            
        // Dereference to get actual value
        value = std::move(*data_queue.front());
        data_queue.pop();
        return true;
    }
    
    // Non-blocking pop returning shared_ptr
    std::shared_ptr<T> try_pop() {
        std::lock_guard<std::mutex> lk(mut);
        
        if(data_queue.empty())
            return std::shared_ptr<T>();
            
        // Just return the stored shared_ptr
        std::shared_ptr<T> res = data_queue.front();
        data_queue.pop();
        return res;
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
};

// Performance comparison class for demonstration
class PerformanceTimer {
    std::chrono::high_resolution_clock::time_point start;
public:
    PerformanceTimer() : start(std::chrono::high_resolution_clock::now()) {}
    
    void report(const std::string& operation) {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << operation << " took: " << duration.count() << " microseconds" << std::endl;
    }
};

// Heavy object to demonstrate performance difference
struct HeavyObject {
    std::vector<int> data;
    
    HeavyObject(int size = 1000) : data(size, 42) {}
    
    // Copy constructor - intentionally expensive
    HeavyObject(const HeavyObject& other) : data(other.data) {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
};

int main() {
    threadsafe_queue_optimized<HeavyObject> optimized_queue;
    
    std::cout << "=== Performance Demonstration ===" << std::endl;
    
    // Test optimized version
    {
        PerformanceTimer timer;
        
        // Push operations - allocation happens outside lock
        for(int i = 0; i < 100; ++i) {
            optimized_queue.push(HeavyObject(100));
        }
        
        timer.report("Optimized push operations (100 items)");
    }
    
    {
        PerformanceTimer timer;
        
        // Pop operations - no allocation during pop
        for(int i = 0; i < 100; ++i) {
            auto item = optimized_queue.try_pop();
        }
        
        timer.report("Optimized pop operations (100 items)");
    }
    
    std::cout << "\n=== Exception Safety Demonstration ===" << std::endl;
    
    // The optimized version is exception-safe because:
    // 1. Memory allocation happens outside the lock
    // 2. If allocation fails, mutex is never locked
    // 3. Only simple pointer operations happen under lock
    
    try {
        // This allocation might throw, but won't affect queue state
        optimized_queue.push(HeavyObject(1000000));  // Might throw bad_alloc
        std::cout << "Large object added successfully" << std::endl;
    } catch(const std::exception& e) {
        std::cout << "Exception caught during push: " << e.what() << std::endl;
        std::cout << "Queue remains in valid state" << std::endl;
    }
    
    return 0;
}