/**
 * 
Task:
    1.
    Implement a SharedCounter class.
    It should have an int counter_ member.
    A std::mutex mutex_ to protect counter_.
    A void increment() method that safely increments counter_.
    A int get_value() method that safely returns counter_.

    2.
    In main, create 10 threads. Each thread should call increment() on a single SharedCounter instance 100,000 times.

    3.
    Ensure the final value of the counter is correct (1,000,000).

    4. 
    Simulate a "Resource A" that is initialized once. 
    Create a function longRunningTask() that simulates work and needs to be guaranteed to run in a separate thread. 
    Use std::jthread for longRunningTask if C++20 is available, otherwise manage std::thread with a custom thread_guard (or similar RAII). 
    The longRunningTask should be "detached-like" in behavior but still cleanly joined or managed.
*/


#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <numeric> // For std::iota if needed, not strictly for this problem

// --- SharedCounter Class ---
class SharedCounter {
public:
    SharedCounter() : counter_(0) {}

    void increment() {
        std::lock_guard<std::mutex> guard(mutex_);
        counter_++;
    }

    int get_value() {
        std::unique_lock<std::mutex> guard(mutex_);
        return counter_;
    }

private:
    int counter_;
    std::mutex mutex_;
};

// --- Long Running Task Simulation ---
// Resource A that needs to be initialized once (simulate with a simple print)
void initializeResourceA() {
    static std::once_flag flag; // Ensure this runs only once
    std::call_once(flag, []() {
        std::cout << "Resource A initialized.\n";
    });
}

// Function for the long-running task
void longRunningTask() {
    initializeResourceA(); // Ensure resource is initialized
    std::cout << "Long running task started.\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Simulate work
    std::cout << "Long running task finished.\n";
}

// --- Custom Thread Guard (if jthread is not used/available) ---
// If you're using C++17 or earlier, implement a basic thread_guard
// for std::thread that joins in its destructor.
#if __cplusplus < 202002L // If C++20 or later, std::jthread simplifies this
class thread_guard {
public:
    explicit thread_guard(std::thread t) : t_(std::move(t)) {}

    ~thread_guard() {
        if (t_.joinable()) {
            t_.join();
        }
    }

    // Disable copy constructor and assignment operator
    thread_guard(const thread_guard&) = delete;
    thread_guard& operator=(const thread_guard&) = delete;

private:
    std::thread t_;
};
#endif


int main() {
    // --- Counter Demonstration ---
    SharedCounter counter;
    const int num_threads = 10;
    const int increments_per_thread = 100000;
    std::vector<std::thread> incrementer_threads;

    for (int i = 0; i < num_threads; ++i) {
        incrementer_threads.emplace_back(&SharedCounter::increment, &counter);
    }

    // TODO: Join all incrementer threads
    for (int i = 0; i < num_threads; ++i) {
        incrementer_threads[i].join();
    }

    std::cout << "Final counter value: " << counter.get_value() << std::endl;
    std::cout << "Expected counter value: " << (long long)num_threads * increments_per_thread << std::endl;

    // --- Long Running Task Demonstration ---
    std::cout << "\nStarting long running task demonstration...\n";
    // TODO: Launch longRunningTask in a separate thread.
    // Use std::jthread if C++20. Otherwise, use thread_guard (defined above)
    // to ensure the thread is joined safely.
    thread_guard long_task_guard(std::thread(longRunningTask));

    std::cout << "Main thread continuing after launching long task.\n";
    // Simulate some main thread work
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::cout << "Main thread finished.\n";

    return 0;
}