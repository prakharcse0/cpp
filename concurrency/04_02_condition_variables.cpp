// 1. The Trio (Always Used Together)
// std::mutex mtx;                    // Protects shared data
// std::condition_variable cv;        // Coordination mechanism  
// bool condition = false;            // Shared state


// 2. Waiter Thread:
// std::unique_lock<std::mutex> lock(mtx);
// cv.wait(lock, []{ return condition; });
// Continues when condition becomes true

// What happens inside wait():
// Check if condition is already true → if yes, return immediately
// If false → unlock mutex and go to sleep
// When notified → wake up and reacquire lock
// Check condition again → if true, return; if false, sleep again


// 3. Notifier Thread:
// {
//     std::lock_guard<std::mutex> lock(mtx);
//     condition = true;              // Change the condition
// }
// cv.notify_one();                   // Wake up waiting thread


// The standard C++ library provides not one but two implementations of a condition variable:
// std::condition_variables and std::condition_variables_any.
// Both of these are declared in the <condition_variable> library header.

// In both cases, they need need to work with a mutex in order to provide appropriate synchronization;
// the former is limited to work with std::mutex,
// whereas the latter can work with anything that meets some critical for being mutex-like,
// hence the _any suffix.
// Because the std::condition_variable_any is more general, 
// there's the potential for addition cost in terms of size, performance, or operating system resource,
// so std::condition_variable should be preferred unless the additional flexibility is required.


#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <chrono>

// Simple data structure to represent a chunk of data
struct data_chunk {
    int id;
    bool is_last;
    
    data_chunk(int i, bool last = false) : id(i), is_last(last) {}
};

// Global variables for simulation
int data_counter = 0;
const int MAX_DATA_ITEMS = 5;

// Forward declarations
bool more_data_to_prepare();
data_chunk prepare_data();
void process(const data_chunk& data);
bool is_last_chunk(const data_chunk& data);


// Global synchronization objects - shared between threads
std::mutex mut;
std::queue<data_chunk> data_queue;
std::condition_variable data_cond;

// Producer thread - prepares data and notifies consumer
void data_preparation_thread() {
    while(more_data_to_prepare()) {
        data_chunk const data = prepare_data();
        
        {   // Explicit scope for critical section
            std::lock_guard<std::mutex> lk(mut);  // RAII lock
            data_queue.push(data);
            
            // notify_one() can be called while holding the lock
            // The waiting thread will still need to reacquire the lock after waking
            data_cond.notify_one();  // Wake up one waiting thread
        }   // lock_guard destructor unlocks here
        
        // Consumer thread can now acquire the lock and process data
    }
    std::cout << "Data preparation thread finished" << std::endl;
}
// Key insight: notify_one() doesn't require the mutex to be unlocked first. 
// The notification just wakes up the waiting thread, but that thread still has to compete for the lock like any other thread.


// Consumer thread - waits for data and processes it
void data_processing_thread() {
    while(true) {
        // IMPORTANT: condition_variable.wait() REQUIRES std::unique_lock
        // - lock_guard cannot be manually unlocked (only unlocks in destructor)
        // - Raw mutex.lock()/unlock() won't work - wait() needs a lock object
        // - wait() needs to unlock/relock the mutex atomically
        // - Only unique_lock provides the flexibility needed for this operation
        std::unique_lock<std::mutex> lk(mut);
        
        // Wait until queue is not empty
        // Condition variable will:
        // 1. Check lambda condition (!data_queue.empty())
        // 2. If false: unlock mutex, sleep, wait for notify
        // 3. When notified: relock mutex, check condition again
        // 4. Return only when condition is true AND mutex is locked
        data_cond.wait(lk, []{return !data_queue.empty();});
        
        // At this point: mutex is locked AND queue has data
        data_chunk data = data_queue.front();
        data_queue.pop();
        
        // Unlock early to minimize lock contention
        // Processing doesn't need the mutex - only queue access does
        lk.unlock();
        
        // Process data outside critical section (good practice)
        process(data);
        
        // Check if we're done
        if(is_last_chunk(data))
            break;
    }
    std::cout << "Data processing thread finished" << std::endl;
}

int main() {
    std::cout << "Starting producer-consumer example with condition variables\n";
    std::cout << "Producer will create " << MAX_DATA_ITEMS << " data items\n\n";
    
    // Create and start both threads
    std::thread producer(data_preparation_thread);
    std::thread consumer(data_processing_thread);
    
    // Wait for both threads to complete
    producer.join();
    consumer.join();
    
    std::cout << "\nAll threads completed successfully!" << std::endl;
    
    return 0;
}

// ========== Helper Function Definitions ==========

// Helper function: checks if there's more data to prepare
bool more_data_to_prepare() {
    return data_counter < MAX_DATA_ITEMS;
}

// Helper function: simulates data preparation
data_chunk prepare_data() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
    data_counter++;
    bool is_last = (data_counter == MAX_DATA_ITEMS);
    std::cout << "Prepared data chunk " << data_counter << std::endl;
    return data_chunk(data_counter, is_last);
}

// Helper function: simulates data processing
void process(const data_chunk& data) {
    std::this_thread::sleep_for(std::chrono::milliseconds(150)); // Simulate work
    std::cout << "Processed data chunk " << data.id << std::endl;
}

// Helper function: checks if this is the last chunk
bool is_last_chunk(const data_chunk& data) {
    return data.is_last;
}