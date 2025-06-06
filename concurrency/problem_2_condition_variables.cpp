// Scenario: Implement a simple thread-safe message queue. 
// One "producer" thread will add messages, and one "consumer" thread will retrieve and process them.

// Task:

// 1.  Implement a ThreadSafeMessageQueue class.
//     It should use std::queue<std::string> internally.
//     A std::mutex to protect the queue.
//     A std::condition_variable to signal when messages are available.
//     A void push(const std::string& message) method: adds a message and notifies consumers.
//     A std::string pop() method: waits until a message is available, then retrieves and returns it.
//     (Optional but good practice): A way to signal shutdown to the consumer. 
// For simplicity, we can let the consumer wait indefinitely or rely on a stop flag. Let's add a void stop() method and a bool stopped_ flag.

// 2.  In main, create one producer thread that pushes 10 messages to the queue.

// 3.  Create one consumer thread that pops and prints these 10 messages.

// 4.  Ensure both threads exit cleanly.


#include <iostream>
#include <thread>
#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>
#include <chrono>

// --- ThreadSafeMessageQueue Class ---
class ThreadSafeMessageQueue {
public:
    void push(const std::string& message) {
        // TODO: Lock, add message, notify
        std::lock_guard<std::mutex> lk(mutex_);
        queue_.push(message);
        cv_.notify_one();
    }

    std::string pop() {
        // TODO: Lock, wait for message, pop, return
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait(lk, [&](){return (!queue_.empty()) || stopped_;});
        if (stopped_ && queue_.empty()) 
            return "STOP_SIGNAL";
        std::string message = queue_.front();
        queue_.pop();
        return message;
    }

    void stop() {
        // TODO: Set stop flag, notify all waiting consumers
        std::lock_guard<std::mutex> lk(mutex_);
        stopped_ = true;
        cv_.notify_all();
    }

private:
    std::queue<std::string> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stopped_ = false; // Flag to signal shutdown to consumer
};

// --- Producer Function ---
void producer(ThreadSafeMessageQueue& q, int num_messages) {
    for (int i = 0; i < num_messages; ++i) {
        std::string message = "Message " + std::to_string(i);
        q.push(message);
        std::cout << "[Producer] Pushed: " << message << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simulate work
    }
    q.stop(); // Signal that no more messages will be produced
}

// --- Consumer Function ---
void consumer(ThreadSafeMessageQueue& q) {
    while (true) {
        std::string message = q.pop();
        if (message == "STOP_SIGNAL") { // Check for a special stop signal
            std::cout << "[Consumer] Received STOP signal. Exiting.\n";
            break;
        }
        std::cout << "[Consumer] Popped: " << message << std::endl;
    }
}

int main() {
    ThreadSafeMessageQueue queue;

    std::thread prod_thread(producer, std::ref(queue), 10); // 10 messages
    std::thread cons_thread(consumer, std::ref(queue));

    prod_thread.join();
    cons_thread.join();

    std::cout << "Producer and Consumer threads finished.\n";

    return 0;
}