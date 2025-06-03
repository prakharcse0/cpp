#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>
#include <chrono>

// Listing 4.4: Extracting push() and wait_and_pop() from listing 4.1
// This shows the core synchronization mechanism using condition variables

template<typename T>
class threadsafe_queue
{
private:
    std::mutex mut;                    // Protects the queue
    std::queue<T> data_queue;          // Underlying storage
    std::condition_variable data_cond; // For blocking operations

public:
    // Thread-safe push operation
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(mut);  // Lock for entire operation
        data_queue.push(new_value);           // Add to queue
        data_cond.notify_one();               // Wake up one waiting thread
    }

    // Blocking pop operation - waits until data is available
    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mut);  // unique_lock needed for condition variable
        
        // Wait until queue is not empty
        // Lambda predicate is checked each time we wake up
        data_cond.wait(lk, [this]{return !data_queue.empty();});
        
        // Queue guaranteed to have data now
        value = data_queue.front();
        data_queue.pop();
    }
};

// Example data structure for demonstration
struct data_chunk {
    int id;
    std::string payload;
    bool is_last;
    
    data_chunk(int i, std::string p, bool last = false) 
        : id(i), payload(p), is_last(last) {}
};

// Global queue instance (as shown in original listing 4.4)
threadsafe_queue<data_chunk> data_queue;  // b - no separate variables needed

// Producer function - prepares and adds data to queue
void data_preparation_thread()
{
    std::cout << "[Producer] Starting data preparation...\n";
    
    for(int i = 1; i <= 5; ++i) {
        // Simulate data preparation work
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        data_chunk data(i, "Payload " + std::to_string(i), i == 5);
        std::cout << "[Producer] Prepared: " << data.payload << std::endl;
        
        data_queue.push(data);  // c - no external synchronization needed
        std::cout << "[Producer] Pushed data to queue\n";
    }
    
    std::cout << "[Producer] Finished preparation\n";
}

// Consumer function - waits for and processes data
void data_processing_thread()
{
    std::cout << "[Consumer] Starting data processing...\n";
    
    while(true)
    {
        data_chunk data(0, "");
        
        std::cout << "[Consumer] Waiting for data...\n";
        data_queue.wait_and_pop(data);  // d - condition variable wait handled internally
        
        std::cout << "[Consumer] Received: " << data.payload << std::endl;
        
        // Simulate processing work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "[Consumer] Processed: " << data.payload << std::endl;
        
        if(data.is_last) {
            std::cout << "[Consumer] Received last chunk, stopping\n";
            break;
        }
    }
}

// Helper functions to match original listing structure
bool more_data_to_prepare() {
    // Simplified - in real code this would check some condition
    return false;  // Not used in this demo
}

data_chunk prepare_data() {
    return data_chunk(0, "dummy");  // Not used in this demo
}

void process(const data_chunk& data) {
    std::cout << "Processing: " << data.payload << std::endl;
}

bool is_last_chunk(const data_chunk& data) {
    return data.is_last;
}

void demonstrate_synchronization() {
    std::cout << "\n=== Synchronization Mechanism Explanation ===\n";
    std::cout << "1. MUTEX protects the queue from concurrent access\n";
    std::cout << "2. CONDITION VARIABLE allows threads to wait efficiently\n";
    std::cout << "3. push() notifies waiting threads when data arrives\n";
    std::cout << "4. wait_and_pop() blocks until data is available\n";
    std::cout << "5. No external synchronization needed!\n\n";
}

void demonstrate_condition_variable_behavior() {
    std::cout << "=== Condition Variable Details ===\n";
    std::cout << "• wait() releases mutex and blocks thread\n";
    std::cout << "• notify_one() wakes up one waiting thread\n";
    std::cout << "• Woken thread re-acquires mutex and checks predicate\n";
    std::cout << "• If predicate false, thread goes back to sleep\n";
    std::cout << "• If predicate true, thread continues with data\n";
    std::cout << "• Predicate: [this]{return !data_queue.empty();}\n\n";
}

int main()
{
    std::cout << "Basic Thread-Safe Queue Implementation\n";
    std::cout << "Extracted push() and wait_and_pop() operations\n";
    std::cout << "==============================================\n\n";
    
    demonstrate_synchronization();
    demonstrate_condition_variable_behavior();
    
    std::cout << "=== Live Demonstration ===\n";
    
    // Start producer and consumer threads
    std::thread producer(data_preparation_thread);
    std::thread consumer(data_processing_thread);
    
    // Wait for both threads to complete
    producer.join();
    consumer.join();
    
    std::cout << "\n=== Key Observations ===\n";
    std::cout << "• Consumer blocks when queue is empty\n";
    std::cout << "• Producer wakes up consumer with notify_one()\n";
    std::cout << "• No race conditions - operations are atomic\n";
    std::cout << "• Mutex and condition variable are encapsulated\n";
    std::cout << "• External code doesn't need synchronization knowledge\n";
    
    return 0;
}