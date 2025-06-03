#include <iostream>
#include <future>
#include <thread>
#include <chrono>

/*
SIMPLEST PROMISE EXAMPLE:
- Thread 1: Waits for a result using future
- Thread 2: Provides the result using promise
- Communication happens when promise.set_value() is called
*/

void worker_thread(std::promise<int> prom) {
    // Simulate some work
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Calculate result
    int result = 42;
    
    // Send result to waiting thread
    prom.set_value(result);
    
    std::cout << "Worker: Result sent!\n";
}

int main() {
    // Create promise/future pair
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    
    std::cout << "Main: Starting worker thread...\n";
    
    // Start worker thread, move promise to it
    std::thread worker(worker_thread, std::move(promise));
    
    std::cout << "Main: Waiting for result...\n";
    
    // Block until result is available
    int result = future.get();
    
    std::cout << "Main: Got result: " << result << "\n";
    
    worker.join();
    return 0;
}

/*
OUTPUT:
Main: Starting worker thread...
Main: Waiting for result...
Worker: Result sent!
Main: Got result: 42

KEY POINTS:
1. promise.get_future() creates the linked future
2. future.get() BLOCKS until set_value() is called
3. Promise must be MOVED to another thread (not copied)
4. One-time use: each promise/future pair works only once
5. If promise is destroyed without set_value(), future.get() throws exception
*/