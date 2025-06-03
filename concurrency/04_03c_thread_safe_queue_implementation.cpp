#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>
#include <chrono>

// Listing 4.5: Full class definition for a thread-safe queue using condition variables
template<typename T>
class threadsafe_queue
{
private:
    mutable std::mutex mut;            // b - Must be mutable for const methods
    std::queue<T> data_queue;
    std::condition_variable data_cond;

public:
    // Default constructor
    threadsafe_queue()
    {}

    // Copy constructor - must lock the source queue
    threadsafe_queue(threadsafe_queue const& other)
    {
        std::lock_guard<std::mutex> lk(other.mut);  // Lock source queue
        data_queue = other.data_queue;              // Copy data
    }

    // Assignment operator deleted for simplicity  
    threadsafe_queue& operator=(const threadsafe_queue&) = delete;

    // Thread-safe push - adds element and notifies waiting threads
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(new_value);
        data_cond.notify_one();  // Wake up one waiting thread
    }

    // Blocking pop - waits until data is available
    // Version 1: Sets value by reference
    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this]{return !data_queue.empty();});
        value = data_queue.front();
        data_queue.pop();
    }

    // Blocking pop - waits until data is available
    // Version 2: Returns shared_ptr (exception-safe)
    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this]{return !data_queue.empty();});
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }

    // Non-blocking pop - returns immediately
    // Version 1: Sets value by reference, returns success status
    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty())
            return false;
        value = data_queue.front();
        data_queue.pop();
        return true;
    }

    // Non-blocking pop - returns immediately  
    // Version 2: Returns shared_ptr (nullptr if empty)
    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut);
        if(data_queue.empty())
            return std::shared_ptr<T>();  // Return null pointer
        std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
        data_queue.pop();
        return res;
    }

    // Check if queue is empty (thread-safe)
    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut);  // Mutable mutex allows locking
        return data_queue.empty();
    }
};

// Test data structure
struct WorkItem {
    int task_id;
    std::string description;
    int processing_time_ms;
    
    WorkItem(int id, std::string desc, int time_ms = 100) 
        : task_id(id), description(desc), processing_time_ms(time_ms) {}
};

// Demonstrate all four pop methods
void demonstrate_all_pop_methods() {
    std::cout << "\n=== Demonstrating All Pop Methods ===\n";
    
    threadsafe_queue<WorkItem> test_queue;
    
    // Add test data
    test_queue.push(WorkItem(1, "Task One", 50));
    test_queue.push(WorkItem(2, "Task Two", 75));
    
    std::cout << "Queue has data. Testing all pop methods:\n\n";
    
    // Method 1: try_pop with reference
    WorkItem item1(0, "");
    if(test_queue.try_pop(item1)) {
        std::cout << "try_pop(ref): Got " << item1.description << std::endl;
    }
    
    // Method 2: try_pop with shared_ptr
    auto item2_ptr = test_queue.try_pop();
    if(item2_ptr) {
        std::cout << "try_pop(ptr): Got " << item2_ptr->description << std::endl;
    }
    
    // Methods 3 & 4: wait_and_pop (will be tested with threading)
    std::cout << "wait_and_pop methods will be demonstrated with threads\n";
}

void demonstrate_blocking_behavior() {
    std::cout << "\n=== Demonstrating Blocking Behavior ===\n";
    
    threadsafe_queue<std::string> demo_queue;
    
    // Consumer thread that waits for data
    std::thread consumer([&demo_queue]() {
        std::cout << "[Consumer] Waiting for data (will block)...\n";
        
        std::string message;
        demo_queue.wait_and_pop(message);  // This will block
        
        std::cout << "[Consumer] Received: " << message << std::endl;
    });
    
    // Let consumer start and begin waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Producer sends data
    std::cout << "[Producer] Sending data to wake up consumer...\n";
    demo_queue.push("Hello from producer!");
    
    consumer.join();
    std::cout << "[Demo] Consumer was successfully woken up\n";
}

void demonstrate_multiple_consumers() {
    std::cout << "\n=== Multiple Consumers Demo ===\n";
    
    threadsafe_queue<int> work_queue;
    
    // Start multiple consumer threads
    auto consumer_func = [&work_queue](int consumer_id) {
        for(int i = 0; i < 3; ++i) {
            int work_item;
            work_queue.wait_and_pop(work_item);
            std::cout << "[Consumer " << consumer_id << "] Processing: " << work_item << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };
    
    std::thread consumer1(consumer_func, 1);
    std::thread consumer2(consumer_func, 2);
    
    // Give consumers time to start waiting
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Producer adds work items
    std::cout << "[Producer] Adding work items...\n";
    for(int i = 1; i <= 6; ++i) {
        work_queue.push(i * 10);
        std::cout << "[Producer] Added: " << (i * 10) << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    consumer1.join();
    consumer2.join();
    std::cout << "[Demo] All consumers finished\n";
}

void demonstrate_copy_constructor() {
    std::cout << "\n=== Copy Constructor Demo ===\n";
    
    threadsafe_queue<std::string> original;
    original.push("First item");
    original.push("Second item");
    
    // Copy constructor - must be thread-safe
    std::cout << "Creating copy of queue...\n";
    threadsafe_queue<std::string> copy(original);
    
    // Verify copy worked
    std::string item;
    if(copy.try_pop(item)) {
        std::cout << "Copy contains: " << item << std::endl;
    }
    
    // Original should still have data
    if(original.try_pop(item)) {
        std::cout << "Original contains: " << item << std::endl;
    }
}

void demonstrate_empty_queue_behavior() {
    std::cout << "\n=== Empty Queue Behavior ===\n";
    
    threadsafe_queue<int> empty_queue;
    
    std::cout << "Queue is empty: " << (empty_queue.empty() ? "true" : "false") << std::endl;
    
    // try_pop should return false/nullptr
    int value;
    if(!empty_queue.try_pop(value)) {
        std::cout << "try_pop(ref) correctly returned false for empty queue\n";
    }
    
    auto ptr = empty_queue.try_pop();
    if(!ptr) {
        std::cout << "try_pop(ptr) correctly returned nullptr for empty queue\n";
    }
}

int main()
{
    std::cout << "Complete Thread-Safe Queue Implementation\n";
    std::cout << "Full implementation with all methods\n";
    std::cout << "====================================\n";
    
    demonstrate_all_pop_methods();
    demonstrate_blocking_behavior();
    demonstrate_multiple_consumers();
    demonstrate_copy_constructor();
    demonstrate_empty_queue_behavior();
    
    std::cout << "\n=== Implementation Features Summary ===\n";
    std::cout << "✅ Four pop methods: try_pop (2 versions) + wait_and_pop (2 versions)\n";
    std::cout << "✅ Thread-safe push with condition variable notification\n";
    std::cout << "✅ Mutable mutex for const method access\n";
    std::cout << "✅ Exception-safe shared_ptr usage\n";
    std::cout << "✅ Thread-safe copy constructor\n";
    std::cout << "✅ Proper blocking/non-blocking behavior\n";
    std::cout << "✅ Multiple consumer support with notify_one()\n";
    
    return 0;
}