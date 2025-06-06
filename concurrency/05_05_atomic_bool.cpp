#include <atomic>
#include <iostream>
#include <thread>
#include <cassert>

int main() {
    // Basic construction and assignment
    // std::atomic<bool> is more featured than std::atomic_flag
    // Not copy-constructible/assignable but can construct from regular bool
    std::atomic<bool> flag1(true);   // Initialize with true
    std::atomic<bool> flag2(false);  // Initialize with false
    
    // Assignment from regular bool - returns bool value, not reference
    // This avoids extra load operations and potential race conditions
    bool result = (flag1 = false);  // Returns the assigned value (false)
    std::cout << "Assignment returned: " << result << std::endl;
    
    // Basic operations: store(), load(), exchange()
    // store() - atomic write operation
    flag1.store(true);  // Can specify memory ordering: store(true, std::memory_order_release)
    
    // load() - atomic read operation  
    bool current = flag1.load();  // Can specify memory ordering: load(std::memory_order_acquire)
    std::cout << "Current value: " << current << std::endl;
    
    // Implicit conversion to bool also works
    if (flag1) {  // Same as flag1.load()
        std::cout << "Flag is true" << std::endl;
    }
    
    // exchange() - atomic read-modify-write operation
    // Sets new value and returns the old value atomically
    bool old_value = flag1.exchange(false);
    std::cout << "Old value was: " << old_value << ", new value: " << flag1.load() << std::endl;
    
    // Compare-and-exchange operations - cornerstone of atomic programming
    // Compares current value with expected, stores desired if they match
    // Returns true if store happened, false otherwise
    
    // compare_exchange_weak() - can fail spuriously on some architectures
    // Must typically be used in a loop
    bool expected = false;
    bool desired = true;
    
    // Spurious failure possible - loop until success or actual mismatch
    while (!flag1.compare_exchange_weak(expected, desired) && !expected) {
        // expected gets updated with actual value on failure
        // Loop continues only if expected is still false (spurious failure)
    }
    std::cout << "After compare_exchange_weak: " << flag1.load() << std::endl;
    
    // compare_exchange_strong() - only fails if values don't match
    // No spurious failures, but may contain internal loop on some platforms
    expected = true;
    desired = false;
    bool success = flag1.compare_exchange_strong(expected, desired);
    std::cout << "Strong exchange success: " << success << ", value: " << flag1.load() << std::endl;
    
    // Memory ordering can be specified for success and failure separately
    expected = false;
    flag1.compare_exchange_weak(expected, true, 
                               std::memory_order_acq_rel,     // success ordering
                               std::memory_order_acquire);    // failure ordering
    
    // If only success ordering specified, failure gets relaxed version
    // memory_order_release -> memory_order_relaxed
    // memory_order_acq_rel -> memory_order_acquire
    flag1.compare_exchange_weak(expected, true, std::memory_order_acq_rel);
    
    // Check if operations are lock-free (implementation dependent)
    // Unlike std::atomic_flag, std::atomic<bool> may use locks internally
    if (flag1.is_lock_free()) {
        std::cout << "atomic<bool> is lock-free on this platform" << std::endl;
    } else {
        std::cout << "atomic<bool> uses locks on this platform" << std::endl;
    }
    
    return 0;
}