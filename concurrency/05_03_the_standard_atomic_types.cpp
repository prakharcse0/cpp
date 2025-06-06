#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

int main() {
    // STANDARD ATOMIC TYPES - found in <atomic> header
    // All operations on these types are atomic by language definition
    
    // 1. STD::ATOMIC_FLAG - The simplest atomic type
    // - Only type guaranteed to be lock-free (no is_lock_free() method needed)
    // - Simple boolean flag: can only be set or cleared
    // - Initialized to clear state
    // - No assignment, copy construction, or other operations
    std::atomic_flag simple_flag = ATOMIC_FLAG_INIT;  // Must initialize to clear
    
    // Only two operations available:
    bool was_set = simple_flag.test_and_set();  // Atomically set flag and return previous value
    simple_flag.clear();                        // Atomically clear the flag
    
    // 2. STD::ATOMIC<T> CLASS TEMPLATE - More full-featured atomic types
    // May or may not be lock-free (check with is_lock_free())
    
    // Basic atomic integer
    std::atomic<int> atomic_int{42};
    
    // Check if operations use actual atomic instructions (true) or internal locks (false)
    if (atomic_int.is_lock_free()) {
        std::cout << "atomic<int> uses hardware atomic instructions\n";
    } else {
        std::cout << "atomic<int> uses compiler/library locks for emulation\n";
    }
    
    // 3. ALTERNATIVE TYPE NAMES - Historical names for atomic specializations
    // Pattern: atomic_T corresponds to std::atomic<T>
    // Note: Mixing alternative names with std::atomic<T> can lead to non-portable code
    
    std::atomic_bool    ab;     // Same as std::atomic<bool>
    std::atomic_char    ac;     // Same as std::atomic<char>
    std::atomic_int     ai;     // Same as std::atomic<int>
    std::atomic_long    al;     // Same as std::atomic<long>
    // Many more available (see text for complete list)
    
    // 4. STANDARD LIBRARY TYPEDEF ATOMICS
    // Pattern: atomic_T for standard typedef T
    std::atomic_size_t      atomic_sz;      // std::atomic<size_t>
    std::atomic_ptrdiff_t   atomic_pd;      // std::atomic<ptrdiff_t>
    std::atomic_intmax_t    atomic_max;     // std::atomic<intmax_t>
    // Plus int_least*_t, int_fast*_t variants
    
    // 5. ATOMIC OPERATIONS - Available methods
    
    // Basic load/store operations
    atomic_int.store(100);              // Atomic store
    int value = atomic_int.load();      // Atomic load
    
    // Assignment and conversion (also atomic)
    atomic_int = 200;                   // Equivalent to store()
    int val2 = atomic_int;              // Equivalent to load()
    
    // Exchange operations
    int old_val = atomic_int.exchange(300);  // Set new value, return old value
    
    // Compare-and-swap operations
    int expected = 300;
    bool success1 = atomic_int.compare_exchange_weak(expected, 400);    // May fail spuriously
    bool success2 = atomic_int.compare_exchange_strong(expected, 500);  // Only fails if values don't match
    
    // Compound assignment operators (return stored value, not reference)
    std::atomic<int> counter{0};
    int result1 = (counter += 5);       // Returns new value (5)
    int result2 = (counter -= 2);       // Returns new value (3)
    
    // Named member functions (return previous value before operation)
    int prev1 = counter.fetch_add(10);  // Returns old value, then adds
    int prev2 = counter.fetch_sub(3);   // Returns old value, then subtracts
    
    // Increment/decrement for integral types and pointers
    std::atomic<int*> ptr_atomic{nullptr};
    ptr_atomic++;                       // Atomic increment
    ptr_atomic--;                       // Atomic decrement
    
    // Bitwise operations (for integral types)
    std::atomic<unsigned> bits{0xFF};
    bits |= 0x0F;                       // Atomic OR assignment
    bits &= 0xF0;                       // Atomic AND assignment
    unsigned old_bits = bits.fetch_or(0x01);   // Named function version
    
    // 6. USER-DEFINED TYPES with std::atomic<>
    // Primary template supports only basic operations
    struct Point { int x, y; };
    std::atomic<Point> atomic_point;
    
    // Only these operations available for user-defined types:
    Point p{1, 2};
    atomic_point.store(p);                              // store()
    Point loaded = atomic_point.load();                 // load()
    Point old_point = atomic_point.exchange(p);         // exchange()
    Point expected_point = p;
    atomic_point.compare_exchange_weak(expected_point, Point{3, 4});    // compare_exchange_*
    
    // 7. MEMORY ORDERING (briefly mentioned - covered in detail in section 5.3)
    // Optional parameter for all atomic operations to specify memory ordering semantics
    
    // Three categories of operations:
    // Store operations: memory_order_relaxed, memory_order_release, memory_order_seq_cst
    atomic_int.store(42, std::memory_order_release);
    
    // Load operations: memory_order_relaxed, memory_order_consume, memory_order_acquire, memory_order_seq_cst  
    int loaded_val = atomic_int.load(std::memory_order_acquire);
    
    // Read-modify-write: all above plus memory_order_acq_rel
    atomic_int.fetch_add(1, std::memory_order_acq_rel);
    
    // Default ordering is sequential consistency (strongest guarantee)
    atomic_int.store(100);  // Same as store(100, std::memory_order_seq_cst)
    
    // 8. KEY PROPERTIES OF ATOMIC TYPES
    // - Not copyable or assignable in conventional sense (no copy constructor/assignment operator)
    // - Support assignment from and conversion to corresponding built-in types  
    // - Operations return values, not references (to avoid race conditions)
    // - Assignment operators return stored value
    // - Named functions (fetch_*) return previous value before operation
    
    // IMPORTANT NOTES:
    // - std::atomic_flag is the only guaranteed lock-free type
    // - Other types may use lock-based emulation (check with is_lock_free())
    // - On popular platforms, built-in type atomics are usually lock-free
    // - Bitwise operations only available where they make sense (not for pointers)
    // - Generic std::atomic<T> template works with user-defined types but with limited operations
    
    return 0;
}