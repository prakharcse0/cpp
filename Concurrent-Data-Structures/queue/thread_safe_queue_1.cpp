#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <iostream>
#include <thread>
#include <chrono>

template<typename T>
class threadsafe_queue {
private:
    // Mutable allows locking in const functions
    mutable std::mutex mut;
    
    // Standard queue for actual data storage
    std::queue<T> data_queue;
    
    // Condition variable for blocking wait operations
    // Allows threads to wait efficiently instead of polling
    std::condition_variable data_cond;

public:
    threadsafe_queue() {}
    
    // Push operation - adds element and notifies waiting threads
    void push(T new_value) {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(std::move(new_value));
        
        // Wake up ONE waiting thread - efficient notification
        // Only one thread needs to wake up since only one item was added
        data_cond.notify_one();
    }
    
    // Blocking pop with reference parameter - waits until data available
    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lk(mut);  // unique_lock for condition variable
        
        // Wait until queue is not empty - lambda checks condition
        // Handles spurious wakeups automatically
        data_cond.wait(lk, [this] { return !data_queue.empty(); });
        
        value = std::move(data_queue.front());
        data_queue.pop();
    }
    
    // Blocking pop returning shared_ptr - alternative interface
    std::shared_ptr<T> wait_and_pop() {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] { return !data_queue.empty(); });
        
        // Create shared_ptr from front element
        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(data_queue.front()))
        );
        data_queue.pop();
        return res;
    }
    
    // Non-blocking pop with reference - returns immediately
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lk(mut);
        
        // Return false if empty instead of blocking
        if(data_queue.empty())
            return false;
            
        value = std::move(data_queue.front());
        data_queue.pop();
        return true;
    }
    
    // Non-blocking pop returning shared_ptr
    std::shared_ptr<T> try_pop() {
        std::lock_guard<std::mutex> lk(mut);
        
        if(data_queue.empty())
            return std::shared_ptr<T>();  // Return null pointer
            
        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(data_queue.front()))
        );
        data_queue.pop();
        return res;
    }
    
    // Check if queue is empty
    bool empty() const {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
};

// Demonstration of producer-consumer pattern
void producer(threadsafe_queue<int>& queue) {
    for(int i = 0; i < 10; ++i) {
        queue.push(i);
        std::cout << "Produced: " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void consumer(threadsafe_queue<int>& queue, int id) {
    for(int i = 0; i < 5; ++i) {
        int value;
        queue.wait_and_pop(value);  // Blocks until data available
        std::cout << "Consumer " << id << " consumed: " << value << std::endl;
    }
}

int main() {
    threadsafe_queue<int> queue;
    
    // Start producer and consumer threads
    std::thread prod(producer, std::ref(queue));
    std::thread cons1(consumer, std::ref(queue), 1);
    std::thread cons2(consumer, std::ref(queue), 2);
    
    prod.join();
    cons1.join();
    cons2.join();
    
    // Demonstrate try_pop
    std::cout << "\n--- Testing try_pop ---" << std::endl;
    queue.push(999);
    
    int value;
    if(queue.try_pop(value)) {
        std::cout << "try_pop succeeded: " << value << std::endl;
    }
    
    if(!queue.try_pop(value)) {
        std::cout << "try_pop failed - queue empty" << std::endl;
    }
    
    return 0;
}