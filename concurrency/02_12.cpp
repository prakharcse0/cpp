#include <vector>
#include <thread>
#include <algorithm>
#include <functional>
#include <iostream>
#include <chrono>

// Worker function that each thread will execute
void do_work(unsigned id) {
    std::cout << "Thread " << id << " starting work\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100 + id * 10));
    std::cout << "Thread " << id << " finished work\n";
}

void f() {
    std::vector<std::thread> threads;
    
    // Spawn 20 threads
    for(unsigned i = 0; i < 20; ++i) {
        threads.push_back(std::thread(do_work, i));
    }
    
    // // Wait for all threads to complete
    // std::for_each(threads.begin(), threads.end(),
    //               std::mem_fn(&std::thread::join));

    // better
    std::for_each(threads.begin(), threads.end(), 
              [](std::thread& t){ t.join(); });
}

int main() {
    std::cout << "Starting thread pool example...\n";
    f();
    std::cout << "All threads completed.\n";
    return 0;
}

// Explanation of std::for_each line:
// std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
// What it does: Calls join() on every thread in the vector.

// Breaking it down:
// std::for_each: Applies a function to each element in a range
// threads.begin(), threads.end(): The range (all threads in vector)
// std::mem_fn(&std::thread::join): Creates a callable wrapper for the member function join()

// Equivalent to:
// for(auto& t : threads) {
//     t.join();
// }

