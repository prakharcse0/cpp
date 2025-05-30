#include <iostream>   // For std::cout
#include <thread>     // For std::thread
#include <mutex>      // For std::mutex, std::lock_guard
#include <deque>      // Default container for std::stack
#include <stdexcept>  // For std::logic_error (if throwing on empty access)
#include <chrono>     // For std::this_thread::sleep_for

template<typename T, typename Container = std::deque<T>>
class my_stack {
private:
    Container c;
    mutable std::mutex m; // Mutex protects internal container 'c'

public:
    my_stack() = default;

    // Protected read: Is the stack empty?
    bool empty() const {
        std::lock_guard<std::mutex> l(m);
        return c.empty();
    }

    // Protected read: Get top element (returns a COPY for safety).
    // Note: This method alone is protected, but its *usage* can still be problematic.
    T top() const {
        std::lock_guard<std::mutex> l(m);
        if (c.empty()) {
            // Best practice for `top()` on empty: throw an exception or return std::optional (C++17)
            throw std::logic_error("top() called on empty stack");
        }
        return c.back(); // Returns a COPY, not a reference/pointer.
    }

    // Protected write: Push element.
    void push(T const& value) {
        std::lock_guard<std::mutex> l(m);
        c.push_back(value);
    }

    // Protected write: Pop element.
    // Note: This method alone is protected, but its *usage* can still be problematic.
    void pop() {
        std::lock_guard<std::mutex> l(m);
        if (c.empty()) {
             throw std::logic_error("pop() called on empty stack");
        }
        c.pop_back();
    }
};

// --- Inherent Race Condition 1: Stale Information ---
// Problematic pattern: Checking `empty()` then acting on `top()`/`pop()`.
void demonstrate_empty_top_pop_race() {
    std::cout << "\n--- DEMO: Stale Information Race (empty() then top()/pop()) ---" << std::endl;
    my_stack<int> s;
    s.push(10); // Stack: [10]

    // Scenario: Thread A checks `empty()`, another Thread B calls `pop()` before A calls `top()`.
    std::thread t_reader([&s]() {
        // Safe in single-threaded code, DANGEROUS in multi-threaded:
        if (!s.empty()) { // Thread A checks empty() -> returns false (stack is not empty).
            std::cout << "[Reader Thread] Stack is not empty. (At time of check)" << std::endl;
            // A critical moment: Another thread could pop the last element *here*.
            // If that happens, the next call to `s.top()` or `s.pop()` will be undefined behavior
            // (or throw an exception, if handled internally).
            try {
                // Simulating concurrent pop by main thread
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                int const value = s.top(); // Thread A calls top().
                std::cout << "[Reader Thread] Successfully got value: " << value << std::endl;
                s.pop(); // Thread A calls pop().
            } catch (const std::logic_error& e) {
                std::cerr << "[Reader Thread ERROR] Caught: " << e.what() << " - Race occurred!" << std::endl;
            }
        } else {
            std::cout << "[Reader Thread] Stack was empty." << std::endl;
        }
    });

    // Simulating another thread (main thread) popping
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Give reader a head start on empty() check
    if (!s.empty()) { // Make sure stack isn't already empty before popping
        s.pop(); // Main thread pops the last element. Stack: []
        std::cout << "[Main Thread] Popped an item concurrently. Stack now empty." << std::endl;
    }

    t_reader.join();
    std::cout << "--- End Stale Information Race Demo ---" << std::endl;
}

// --- Inherent Race Condition 2: Lost Updates/Double Processing ---
// Problematic pattern: Multiple threads performing top() then pop() on same shared stack.
void demonstrate_top_pop_race() {
    std::cout << "\n--- DEMO: Lost Update / Double Processing Race (top() then pop()) ---" << std::endl;
    my_stack<int> s;
    s.push(1);
    s.push(2); // Stack: [1, 2]

    auto worker = [&](int thread_id) {
        // Each call to top() and pop() is individually protected by mutex.
        // But the sequence of calls is NOT atomic.
        try {
            if (!s.empty()) { // Could add empty check, but the core issue is between top/pop
                int const value = s.top(); // Thread gets top value (e.g., 2)
                std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Simulate other thread interleaving
                s.pop(); // Thread pops (removes 2, or 1 if other thread popped 2)
                std::cout << "[Thread " << thread_id << "] Processed value: " << value << std::endl;
            } else {
                std::cout << "[Thread " << thread_id << "] Stack was empty, nothing to process." << std::endl;
            }
        } catch (const std::logic_error& e) {
            std::cerr << "[Thread " << thread_id << " ERROR] " << e.what() << " - Race occurred!" << std::endl;
        }
    };

    std::thread tA(worker, 1);
    std::thread tB(worker, 2);

    tA.join();
    tB.join();

    // Possible outcome (depends on timing, but demonstrates the problem):
    // If Thread A calls top(), gets 2.
    // If Thread B calls top(), also gets 2 (before A calls pop()).
    // Then Thread A calls pop(), removes 2. Stack is [1].
    // Then Thread B calls pop(), removes 1. Stack is [].
    // Result: Both threads processed '2', and '1' was discarded without processing.
    // This is an incorrect outcome (a logical bug) due to the interface design.

    std::cout << "--- End Lost Update / Double Processing Race Demo ---" << std::endl;
}

// --- Solutions to Inherent Interface Races ---
// 1. Combine operations: Make a single, atomic operation that gets and removes.
//    Example: `std::optional<T> try_pop();` or `void pop(T& value);`
//    Challenges: Exception safety if copying the return value can throw (`std::bad_alloc`).
//    `std::optional` or `std::unique_ptr` as return types can mitigate this.
// 2. Throw exceptions: `top()`/`pop()` throw if stack is empty. (Makes `empty()` redundant for safety checks).

int main() {
    demonstrate_empty_top_pop_race();
    demonstrate_top_pop_race();
    return 0;
}
