#include <atomic>
#include <iostream>
#include <cassert>

int main() {
    // Standard atomic integral types: int, unsigned int, long, unsigned long long, etc.
    // All have the same interface - using int as example
    std::atomic<int> atomic_int(10);
    std::atomic<unsigned long long> atomic_ull(100);
    
    // Basic operations same as atomic<bool>: load(), store(), exchange(), compare_exchange_*()
    std::cout << "Initial value: " << atomic_int.load() << std::endl;
    
    atomic_int.store(20);
    int old_val = atomic_int.exchange(30);
    std::cout << "After exchange: old=" << old_val << ", new=" << atomic_int.load() << std::endl;
    
    // Arithmetic operations - comprehensive set available
    // fetch_add(), fetch_sub() - atomic addition/subtraction, return OLD value
    
    atomic_int.store(50);
    int previous = atomic_int.fetch_add(10);  // Add 10, return old value
    assert(previous == 50);                   // Returns original value
    assert(atomic_int.load() == 60);         // New value is 50 + 10
    std::cout << "fetch_add(10): old=" << previous << ", new=" << atomic_int.load() << std::endl;
    
    previous = atomic_int.fetch_sub(5);      // Subtract 5, return old value
    assert(previous == 60);                  // Returns value before subtraction
    assert(atomic_int.load() == 55);        // New value is 60 - 5
    std::cout << "fetch_sub(5): old=" << previous << ", new=" << atomic_int.load() << std::endl;
    
    // Bitwise operations - fetch_and(), fetch_or(), fetch_xor()
    // All return OLD value before operation
    
    atomic_int.store(0b1111);  // Binary 15
    previous = atomic_int.fetch_and(0b1010);  // AND with 10, return old
    assert(previous == 15);                   // Returns original value
    assert(atomic_int.load() == 10);         // 15 & 10 = 10
    std::cout << "fetch_and: old=" << previous << ", new=" << atomic_int.load() << std::endl;
    
    previous = atomic_int.fetch_or(0b0101);   // OR with 5, return old
    assert(previous == 10);                   // Returns value before OR
    assert(atomic_int.load() == 15);         // 10 | 5 = 15
    std::cout << "fetch_or: old=" << previous << ", new=" << atomic_int.load() << std::endl;
    
    previous = atomic_int.fetch_xor(0b1111);  // XOR with 15, return old
    assert(previous == 15);                   // Returns value before XOR
    assert(atomic_int.load() == 0);          // 15 ^ 15 = 0
    std::cout << "fetch_xor: old=" << previous << ", new=" << atomic_int.load() << std::endl;
    
    // Compound assignment operators - return NEW value (unlike fetch_* functions)
    // +=, -=, &=, |=, ^=
    
    atomic_int.store(100);
    int new_val = (atomic_int += 25);  // Add 25, return new value
    assert(new_val == 125);
    assert(atomic_int.load() == 125);
    std::cout << "+= operator: result=" << new_val << std::endl;
    
    new_val = (atomic_int -= 15);      // Subtract 15, return new value
    assert(new_val == 110);
    std::cout << "-= operator: result=" << new_val << std::endl;
    
    atomic_int.store(0b1111);  // Reset to 15
    new_val = (atomic_int &= 0b1100);  // AND with 12, return new value
    assert(new_val == 12);             // 15 & 12 = 12
    std::cout << "&= operator: result=" << new_val << std::endl;
    
    new_val = (atomic_int |= 0b0011);  // OR with 3, return new value
    assert(new_val == 15);             // 12 | 3 = 15
    std::cout << "|= operator: result=" << new_val << std::endl;
    
    new_val = (atomic_int ^= 0b1010);  // XOR with 10, return new value
    assert(new_val == 5);              // 15 ^ 10 = 5
    std::cout << "^= operator: result=" << new_val << std::endl;
    
    // Increment/decrement operators
    // Pre-increment/decrement: ++x, --x (return new value)
    // Post-increment/decrement: x++, x-- (return old value)
    
    atomic_int.store(10);
    
    new_val = ++atomic_int;            // Pre-increment: increment then return
    assert(new_val == 11);
    assert(atomic_int.load() == 11);
    std::cout << "Pre-increment: " << new_val << std::endl;
    
    int old_increment = atomic_int++;  // Post-increment: return then increment
    assert(old_increment == 11);       // Returns value before increment
    assert(atomic_int.load() == 12);  // Value was incremented
    std::cout << "Post-increment: returned=" << old_increment << ", current=" << atomic_int.load() << std::endl;
    
    new_val = --atomic_int;            // Pre-decrement: decrement then return
    assert(new_val == 11);
    std::cout << "Pre-decrement: " << new_val << std::endl;
    
    int old_decrement = atomic_int--;  // Post-decrement: return then decrement
    assert(old_decrement == 11);       // Returns value before decrement
    assert(atomic_int.load() == 10);  // Value was decremented
    std::cout << "Post-decrement: returned=" << old_decrement << ", current=" << atomic_int.load() << std::endl;
    
    // Memory ordering can be specified for fetch_* functions
    atomic_int.fetch_add(5, std::memory_order_acq_rel);
    atomic_int.fetch_and(0xFF, std::memory_order_release);
    
    // Operator forms always use memory_order_seq_cst
    // For custom memory ordering, use fetch_* functions
    
    // Note: Missing operations compared to regular integers
    // No: *=, /=, %=, <<=, >>= (division, multiplication, shift operators)
    // Atomic integrals typically used as counters or bitmasks, so this is rarely needed
    // For complex operations, use compare_exchange_weak() in a loop:
    
    // Example: atomic multiplication using compare_exchange
    int expected, desired;
    do {
        expected = atomic_int.load();
        desired = expected * 2;  // Calculate new value
    } while (!atomic_int.compare_exchange_weak(expected, desired));
    std::cout << "Atomic multiplication result: " << atomic_int.load() << std::endl;
    
    // All return values are plain integral types (not references to atomic)
    // This allows code to use previous values without additional loads
    
    // Lock-free check
    if (atomic_int.is_lock_free()) {
        std::cout << "atomic<int> is lock-free on this platform" << std::endl;
    }
    
    return 0;
}