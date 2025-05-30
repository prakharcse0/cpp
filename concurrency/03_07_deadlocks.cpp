#include <iostream>  // For std::cout
#include <mutex>     // For std::mutex, std::lock, std::lock_guard, std::adopt_lock
#include <thread>    // For std::thread
#include <string>    // For std::string
#include <chrono>    // For std::chrono::milliseconds

// --- PROBLEM: DEADLOCK ---

// Analogy: Two children, one toy drum, one drumstick. Both want to play.
// Child 1 finds drum, Child 2 finds drumstick.
// Result: Each holds their piece, waits for the other's piece. Neither plays.

// In Multithreading: Two threads, two mutexes.
// Each thread needs to lock both mutexes to perform an operation.
// Scenario:
//   - Thread A locks Mutex M1.
//   - Thread B locks Mutex M2.
//   - Thread A tries to lock Mutex M2 (which B holds) -> BLOCKS.
//   - Thread B tries to lock Mutex M1 (which A holds) -> BLOCKS.
// Result: Neither thread can proceed. This is **DEADLOCK**.
// Deadlock is the biggest problem when an operation requires locking two or more mutexes.

// Common Advice to Avoid Deadlock: Always lock mutexes in the same order.
// Example: If an operation needs Mutex A and Mutex B, always lock A, then B.
// Problem with this advice: Not always straightforward.
//   - Example: Swapping data between two instances of the *same class*.
//     `swap(X& lhs, X& rhs)` needs to lock `lhs.m` and `rhs.m`.
//     If `swap(obj1, obj2)` locks `obj1.m` then `obj2.m`,
//     and `swap(obj2, obj1)` (from another thread) locks `obj2.m` then `obj1.m`,
//     you still get deadlock due to swapped parameter order.

// --- SOLUTION: std::lock() ---

// std::lock() is a C++ Standard Library function designed to lock two or more
// mutexes *without risk of deadlock*. It handles the locking order internally
// to prevent circular dependencies.

// --- Example: Using std::lock() for a thread-safe swap operation ---

// A dummy class representing some large object to be swapped.
class some_big_object {
public:
    int id;
    std::string data;
    some_big_object(int _id = 0, std::string _data = "") : id(_id), data(_data) {}
    void print() const { std::cout << "  BigObject(ID:" << id << ", Data:'" << data << "')" << std::endl; }
};

// Generic swap function for some_big_object
void swap(some_big_object& lhs, some_big_object& rhs) {
    // This is the actual data swap logic, assumed to be thread-safe on its own
    // once protected by external mutexes.
    std::swap(lhs.id, rhs.id);
    std::swap(lhs.data, rhs.data);
}

class X {
private:
    some_big_object some_detail; // Data protected by the mutex
    std::mutex m;                // Mutex for this instance's data

public:
    X(some_big_object const& sd) : some_detail(sd) {}

    // Friend function to allow access to private members for swapping.
    friend void swap(X& lhs, X& rhs) {
        // Step 1: Check for same instance (self-assignment).
        // Attempting to lock a mutex you already own (non-recursive mutex) is Undefined Behavior.
        if (&lhs == &rhs) {
            std::cout << "  [Swap] Attempted to swap object with itself. Returning." << std::endl;
            return;
        }

        std::cout << "  [Swap] Locking mutexes for X objects..." << std::endl;

        // Step 2: Use std::lock() to acquire locks on multiple mutexes simultaneously without deadlock.
        // If std::lock acquires some but fails on others, it automatically releases acquired locks.
        std::lock(lhs.m, rhs.m); // B: Locks lhs.m and rhs.m together.

        // Step 3: Construct std::lock_guard instances.
        // C, D: std::adopt_lock tells lock_guard that the mutex is *already locked*
        //      and it should just take ownership, not try to lock it again.
        //      This ensures mutexes are correctly unlocked (RAII) when lock_a/lock_b go out of scope.
        std::lock_guard<std::mutex> lock_a(lhs.m, std::adopt_lock);
        std::lock_guard<std::mutex> lock_b(rhs.m, std::adopt_lock);

        // Step 4: Perform the actual data swap while both mutexes are held.
        std::cout << "  [Swap] Both mutexes locked. Performing data swap." << std::endl;
        swap(lhs.some_detail, rhs.some_detail);
        std::cout << "  [Swap] Data swapped. Mutexes will unlock on exit." << std::endl;
    }

    // Helper to view content
    void print_details() const {
        std::cout << "X instance details: ";
        some_detail.print();
    }
};

// --- Main Demonstration ---
int main() {
    std::cout << "--- Deadlock & std::lock() Demo ---" << std::endl;

    X obj1(some_big_object(1, "Data A"));
    X obj2(some_big_object(2, "Data B"));

    std::cout << "\nInitial states:" << std::endl;
    obj1.print_details();
    obj2.print_details();

    // Scenario 1: Simple swap
    std::cout << "\nScenario 1: Simple swap(obj1, obj2)" << std::endl;
    swap(obj1, obj2);
    std::cout << "After simple swap:" << std::endl;
    obj1.print_details();
    obj2.print_details();

    // Scenario 2: Concurrent swapped order attempts (without std::lock, would deadlock)
    std::cout << "\nScenario 2: Concurrent swaps (simulating potential deadlock)" << std::endl;
    std::thread t1([&]() {
        std::cout << "[Thread 1] Attempting swap(obj1, obj2)..." << std::endl;
        swap(obj1, obj2);
        std::cout << "[Thread 1] swap(obj1, obj2) finished." << std::endl;
    });

    std::thread t2([&]() {
        // Introduce a slight delay to increase chance of interleaving,
        // but std::lock handles it robustly.
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::cout << "[Thread 2] Attempting swap(obj2, obj1)..." << std::endl;
        swap(obj2, obj1); // Swapped order of parameters
        std::cout << "[Thread 2] swap(obj2, obj1) finished." << std::endl;
    });

    t1.join();
    t2.join();

    std::cout << "\nAfter concurrent swaps:" << std::endl;
    obj1.print_details();
    obj2.print_details();

    // Scenario 3: Swapping an object with itself (handled by check)
    std::cout << "\nScenario 3: Swapping obj1 with obj1" << std::endl;
    swap(obj1, obj1);
    obj1.print_details();


    // --- Important Considerations ---
    // 1. std::lock() only helps when mutexes are acquired *together*.
    //    It doesn't solve deadlocks if mutexes are acquired separately in different functions/paths.
    // 2. Deadlocks are notoriously hard to debug due to their unpredictable nature.
    // 3. Simple rules for deadlock-free code often involve consistent ordering
    //    (where std::lock cannot be used) and avoiding unnecessary mutex holding.

    std::cout << "\n--- Demo End ---" << std::endl;
    return 0;
}