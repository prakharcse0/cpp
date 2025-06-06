#include <atomic>
#include <thread>
#include <iostream>
#include <vector>
#include <mutex>

// SPINLOCK MUTEX IMPLEMENTATION using std::atomic_flag
// Demonstrates practical use of atomic_flag as building block
class spinlock_mutex {
    std::atomic_flag flag;
public:
    // Constructor must initialize flag to clear state
    spinlock_mutex() : flag(ATOMIC_FLAG_INIT) {}
    
    void lock() {
        // Busy-wait loop: keep trying test_and_set until we get false (was clear, now set)
        // acquire semantics ensure operations after lock() don't move before it
        while(flag.test_and_set(std::memory_order_acquire));
    }
    
    void unlock() {
        // Clear the flag to unlock
        // release semantics ensure operations before unlock() don't move after it
        flag.clear(std::memory_order_release);
    }
};

int main() {
    // STD::ATOMIC_FLAG - Simplest standard atomic type representing Boolean flag
    // Deliberately basic, intended as building block only
    // Only atomic type guaranteed to be lock-free
    // Can be in one of two states: set or clear
    
    // INITIALIZATION - Must use ATOMIC_FLAG_INIT macro
    // Always initializes to clear state - no choice in the matter
    // This applies wherever declared and whatever scope
    std::atomic_flag f = ATOMIC_FLAG_INIT;
    
    // If static storage duration, guaranteed statically initialized (no initialization order issues)
    static std::atomic_flag static_flag = ATOMIC_FLAG_INIT;
    
    // THREE OPERATIONS AVAILABLE:
    // 1. Destroy (destructor) - automatic
    // 2. clear() - store operation
    // 3. test_and_set() - read-modify-write operation
    
    // CLEAR OPERATION - store operation
    // Can specify memory order (but not acquire or acq_rel - those are for loads)
    f.clear();                                    // Default: memory_order_seq_cst  
    f.clear(std::memory_order_release);          // Explicit release semantics
    // f.clear(std::memory_order_acquire);       // ERROR: acquire not valid for store
    
    // TEST_AND_SET OPERATION - read-modify-write operation  
    // Atomically sets flag and returns previous value
    // Can use any memory ordering tag
    bool was_set1 = f.test_and_set();                           // Default: seq_cst
    bool was_set2 = f.test_and_set(std::memory_order_acquire);  // Explicit acquire
    bool was_set3 = f.test_and_set(std::memory_order_acq_rel);  // Read-modify-write can use acq_rel
    
    std::cout << "was_set1: " << was_set1 << " (first call after clear)" << std::endl;
    std::cout << "was_set2: " << was_set2 << " (second call - flag already set)" << std::endl;
    std::cout << "was_set3: " << was_set3 << " (third call - flag still set)" << std::endl;
    
    // EXAMPLE: Basic flag usage pattern
    std::atomic_flag example_flag = ATOMIC_FLAG_INIT;
    
    // Initially clear, so first test_and_set returns false
    bool first_call = example_flag.test_and_set();   // Returns false (was clear)
    bool second_call = example_flag.test_and_set();  // Returns true (was set)
    
    std::cout << "\nBasic usage pattern:" << std::endl;
    std::cout << "first_call: " << first_call << " (flag was clear)" << std::endl;
    std::cout << "second_call: " << second_call << " (flag was already set)" << std::endl;
    
    example_flag.clear();                            // Reset to clear
    bool third_call = example_flag.test_and_set();   // Returns false again
    std::cout << "third_call: " << third_call << " (after clear, flag was clear again)" << std::endl;
    
    // COPY/ASSIGNMENT RESTRICTIONS
    // Cannot copy-construct or assign atomic_flag (common to all atomic types)
    // Reason: would involve two separate operations on two objects - can't be atomic
    // std::atomic_flag f2 = f;          // ERROR: no copy constructor
    // std::atomic_flag f3(f);           // ERROR: no copy constructor  
    // f3 = f;                           // ERROR: no assignment operator
    
    // PRACTICAL DEMONSTRATION: Using spinlock mutex
    spinlock_mutex spin_mutex;
    int shared_counter = 0;
    const int num_threads = 4;
    const int increments_per_thread = 1000;
    
    std::vector<std::thread> threads;
    
    // Launch threads that increment shared counter using spinlock
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < increments_per_thread; ++j) {
                // Use RAII lock guard with our spinlock
                std::lock_guard<spinlock_mutex> lock(spin_mutex);
                ++shared_counter;  // Protected by spinlock
            }
        });
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Expected: " << (num_threads * increments_per_thread) << std::endl;
    std::cout << "Actual: " << shared_counter << std::endl;
    
    // LIMITATIONS OF std::atomic_flag:
    // - No simple non-modifying query operation (can't just check if set without changing it)
    // - Very limited feature set compared to std::atomic<bool>
    // - Only suitable as building block for more complex constructs
    // - Spinlock busy-waits, poor choice under contention
    // - Better to use std::atomic<bool> for general Boolean flag needs
    
    // MEMORY ORDERING SUMMARY:
    // clear() - store operation: relaxed, release, seq_cst (default)
    // test_and_set() - read-modify-write: all ordering options available (default seq_cst)
    // Spinlock uses acquire/release for proper synchronization
    
    return 0;
}