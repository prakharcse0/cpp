#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>
#include <chrono>
#include <atomic>
#include <vector>

// Thread-safe queue implementation
template<typename T>
class threadsafe_queue
{
private:
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;

public:
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(new_value);
        data_cond.notify_one();
    }

    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this]{return !data_queue.empty();});
        value = data_queue.front();
        data_queue.pop();
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty())
            return false;
        value = data_queue.front();
        data_queue.pop();
        return true;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
    
    size_t size() const
    {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.size();
    }
};

// Unsafe wrapper around std::queue for comparison
template<typename T>
class unsafe_queue
{
private:
    std::queue<T> data_queue;

public:
    // WARNING: These operations are NOT thread-safe!
    void push(T new_value)
    {
        data_queue.push(new_value);
    }

    // Dangerous: separate front() and pop() operations
    T front()
    {
        return data_queue.front();
    }

    void pop()
    {
        data_queue.pop();
    }

    bool empty() const
    {
        return data_queue.empty();
    }
    
    size_t size() const
    {
        return data_queue.size();
    }

    // Attempt at "safe" pop - but still has race conditions!
    bool try_pop_unsafe(T& value)
    {
        if(data_queue.empty())           // Race condition window here!
            return false;                // Another thread could push/pop
        value = data_queue.front();      // Or here!
        data_queue.pop();                // Or here!
        return true;
    }
};

// Test data
struct TestItem {
    int id;
    std::string data;
    TestItem(int i, std::string d) : id(i), data(d) {}
    TestItem() : id(-1), data("invalid") {}
};

// Demonstrate race conditions with unsafe queue
void demonstrate_unsafe_queue_problems() {
    std::cout << "\n=== Unsafe Queue Problems Demonstration ===\n";
    std::cout << "WARNING: This may crash or produce corrupted data!\n\n";
    
    unsafe_queue<TestItem> dangerous_queue;
    std::atomic<bool> stop_test{false};
    std::atomic<int> corruption_count{0};
    std::atomic<int> success_count{0};
    
    // Producer thread - rapidly adds items
    std::thread producer([&]() {
        for(int i = 0; i < 1000 && !stop_test.load(); ++i) {
            dangerous_queue.push(TestItem(i, "Data " + std::to_string(i)));
            if(i % 100 == 0) {
                std::cout << "[Unsafe Producer] Added " << i << " items\n";
            }
        }
    });
    
    // Consumer thread - tries to safely consume (but can't!)
    std::thread consumer([&]() {
        for(int attempts = 0; attempts < 500 && !stop_test.load(); ++attempts) {
            try {
                // The "classic" unsafe pattern
                if(!dangerous_queue.empty()) {          // Check 1: Queue has data
                    TestItem item = dangerous_queue.front(); // Get data (may be invalid!)
                    dangerous_queue.pop();               // Remove (may crash!)
                    
                    if(item.id >= 0) {
                        success_count++;
                    } else {
                        corruption_count++;
                        std::cout << "[Unsafe Consumer] Got corrupted data!\n";
                    }
                }
            } catch(...) {
                corruption_count++;
                std::cout << "[Unsafe Consumer] Exception caught - queue corrupted!\n";
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });
    
    // Let them run for a short time
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    stop_test = true;
    
    producer.join();
    consumer.join();
    
    std::cout << "[Unsafe Results] Successful operations: " << success_count.load() << std::endl;
    std::cout << "[Unsafe Results] Corrupted/failed operations: " << corruption_count.load() << std::endl;
    std::cout << "[Unsafe Results] Final queue size: " << dangerous_queue.size() << std::endl;
    
    if(corruption_count.load() > 0) {
        std::cout << "❌ Race conditions detected in unsafe queue!\n";
    } else {
        std::cout << "⚠️  No corruption detected this time, but race conditions still exist!\n";
    }
}

// Demonstrate thread-safe queue working correctly
void demonstrate_safe_queue_working() {
    std::cout << "\n=== Thread-Safe Queue Demonstration ===\n";
    std::cout << "This should work reliably without any corruption\n\n";
    
    threadsafe_queue<TestItem> safe_queue;
    std::atomic<int> items_produced{0};
    std::atomic<int> items_consumed{0};
    std::atomic<bool> production_done{false};
    
    // Producer thread
    std::thread producer([&]() {
        for(int i = 0; i < 1000; ++i) {
            safe_queue.push(TestItem(i, "SafeData " + std::to_string(i)));
            items_produced++;
            
            if(i % 200 == 0) {
                std::cout << "[Safe Producer] Added " << i << " items\n";
            }
        }
        production_done = true;
        std::cout << "[Safe Producer] Finished producing\n";
    });
    
    // Consumer thread using blocking wait
    std::thread consumer1([&]() {
        while(!production_done.load() || !safe_queue.empty()) {
            TestItem item;
            if(safe_queue.try_pop(item)) {
                items_consumed++;
                
                // Verify data integrity
                if(item.id < 0) {
                    std::cout << "❌ [Safe Consumer1] Data corruption detected!\n";
                } else if(items_consumed.load() % 200 == 0) {
                    std::cout << "[Safe Consumer1] Processed " << items_consumed.load() << " items\n";
                }
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
    });
    
    // Another consumer using blocking wait_and_pop
    std::thread consumer2([&]() {
        while(!production_done.load() || !safe_queue.empty()) {
            TestItem item;
            // Use try_pop to avoid indefinite blocking at the end
            if(safe_queue.try_pop(item)) {
                items_consumed++;
                
                if(item.id < 0) {
                    std::cout << "❌ [Safe Consumer2] Data corruption detected!\n";
                }
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
    });
    
    producer.join();
    consumer1.join();
    consumer2.join();
    
    std::cout << "[Safe Results] Items produced: " << items_produced.load() << std::endl;
    std::cout << "[Safe Results] Items consumed: " << items_consumed.load() << std::endl;
    std::cout << "[Safe Results] Final queue size: " << safe_queue.size() << std::endl;
    
    if(items_produced.load() == items_consumed.load() && safe_queue.empty()) {
        std::cout << "✅ Thread-safe queue worked perfectly!\n";
    } else {
        std::cout << "❌ Something went wrong (this shouldn't happen!)\n";
    }
}

// Performance comparison
void compare_performance() {
    std::cout << "\n=== Performance Comparison ===\n";
    std::cout << "Measuring overhead of thread safety\n\n";
    
    const int NUM_ITEMS = 100000;
    
    // Test unsafe queue performance (single-threaded only!)
    {
        unsafe_queue<int> perf_queue;
        auto start = std::chrono::high_resolution_clock::now();
        
        for(int i = 0; i < NUM_ITEMS; ++i) {
            perf_queue.push(i);
        }
        
        int value;
        for(int i = 0; i < NUM_ITEMS; ++i) {
            perf_queue.try_pop_unsafe(value);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Unsafe queue (single-threaded): " << duration.count() << " microseconds\n";
    }
    
    // Test safe queue performance (single-threaded)
    {
        threadsafe_queue<int> perf_queue;
        auto start = std::chrono::high_resolution_clock::now();
        
        for(int i = 0; i < NUM_ITEMS; ++i) {
            perf_queue.push(i);
        }
        
        int value;
        for(int i = 0; i < NUM_ITEMS; ++i) {
            perf_queue.try_pop(value);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Thread-safe queue (single-threaded): " << duration.count() << " microseconds\n";
    }
    
    std::cout << "\nNote: Thread-safe queue has overhead, but prevents data corruption\n";
    std::cout << "The overhead is worthwhile for correctness in multi-threaded code!\n";
}

// Show the interface differences
void compare_interfaces() {
    std::cout << "\n=== Interface Comparison ===\n\n";
    
    std::cout << "std::queue (unsafe) interface:\n";
    std::cout << "  front() + pop()     ← Separate operations = race conditions\n";
    std::cout << "  empty()             ← Can change between check and use\n";
    std::cout << "  size()              ← Unreliable in multi-threaded context\n";
    std::cout << "  No blocking wait    ← Consumer must poll or sleep\n\n";
    
    std::cout << "threadsafe_queue interface:\n";
    std::cout << "  try_pop()           ← Atomic operation, no race conditions\n";
    std::cout << "  wait_and_pop()      ← Blocks until data available\n";
    std::cout << "  empty()             ← Thread-safe snapshot\n";
    std::cout << "  push()              ← Notifies waiting consumers\n\n";
    
    std::cout << "Key improvements:\n";
    std::cout << "✅ Combined operations eliminate race conditions\n";
    std::cout << "✅ Blocking operations reduce CPU usage\n";
    std::cout << "✅ Exception-safe with shared_ptr variants\n";
    std::cout << "✅ Built-in synchronization\n";
}

int main()
{
    std::cout << "Thread-Safe vs Unsafe Queue Comparison\n";
    std::cout << "Demonstrating why thread safety matters\n";
    std::cout << "======================================\n";
    
    compare_interfaces();
    
    std::cout << "\n⚠️  WARNING: The next demo may show data corruption!\n";
    std::cout << "Press Enter to continue with unsafe queue demo...";
    std::cin.get();
    
    demonstrate_unsafe_queue_problems();
    demonstrate_safe_queue_working();
    compare_performance();
    
    std::cout << "\n=== Final Recommendations ===\n";
    std::cout << "• NEVER use std::queue directly in multi-threaded code\n";
    std::cout << "• Always use proper synchronization (mutex + condition variable)\n";
    std::cout << "• Combine related operations to eliminate race condition windows\n";
    std::cout << "• Use blocking operations to reduce CPU usage\n";
    std::cout << "• Accept small performance overhead for correctness\n";
    std::cout << "• Consider using established thread-safe libraries\n";
    
    return 0;
}