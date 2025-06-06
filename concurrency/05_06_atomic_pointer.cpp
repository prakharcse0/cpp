#include <atomic>
#include <iostream>
#include <cassert>

class Foo {
public:
    int value;
    Foo(int v = 0) : value(v) {}
};

int main() {
    // std::atomic<T*> - atomic pointer operations
    // Same interface as atomic<bool> but works with pointers
    // Not copy-constructible/assignable, but can construct/assign from T*
    
    Foo some_array[5] = {Foo(0), Foo(1), Foo(2), Foo(3), Foo(4)};
    std::atomic<Foo*> atomic_ptr(some_array);  // Initialize with pointer
    
    // Basic operations: load(), store(), exchange(), compare_exchange_*()
    // All work the same as atomic<bool> but with pointer types
    
    Foo* current = atomic_ptr.load();
    std::cout << "Current points to element with value: " << current->value << std::endl;
    
    atomic_ptr.store(&some_array[1]);
    
    Foo* old_ptr = atomic_ptr.exchange(&some_array[2]);
    std::cout << "Exchanged - old pointed to: " << old_ptr->value 
              << ", now points to: " << atomic_ptr.load()->value << std::endl;
    
    // Pointer arithmetic operations - unique to atomic<T*>
    // fetch_add() and fetch_sub() - atomic pointer arithmetic
    
    atomic_ptr.store(some_array);  // Reset to start of array
    
    // fetch_add() - adds to pointer, returns OLD value
    // This is exchange-and-add operation (atomic read-modify-write)
    Foo* old_value = atomic_ptr.fetch_add(2);  // Move 2 elements forward
    assert(old_value == some_array);           // Returns original position
    assert(atomic_ptr.load() == &some_array[2]); // Now points to 3rd element
    std::cout << "fetch_add(2): old pointed to " << old_value->value 
              << ", now points to " << atomic_ptr.load()->value << std::endl;
    
    // fetch_sub() - subtracts from pointer, returns OLD value
    old_value = atomic_ptr.fetch_sub(1);       // Move 1 element back
    assert(old_value == &some_array[2]);       // Returns previous position
    assert(atomic_ptr.load() == &some_array[1]); // Now points to 2nd element
    std::cout << "fetch_sub(1): old pointed to " << old_value->value 
              << ", now points to " << atomic_ptr.load()->value << std::endl;
    
    // Operator overloads for convenience
    // +=, -=, ++, -- (both pre and post)
    
    // += operator returns NEW value (unlike fetch_add)
    Foo* new_ptr = (atomic_ptr += 2);  // Move 2 elements forward
    assert(new_ptr == &some_array[3]);
    assert(atomic_ptr.load() == &some_array[3]);
    std::cout << "+= operator: now points to " << new_ptr->value << std::endl;
    
    // -= operator returns NEW value (unlike fetch_sub)
    new_ptr = (atomic_ptr -= 1);       // Move 1 element back  
    assert(new_ptr == &some_array[2]);
    assert(atomic_ptr.load() == &some_array[2]);
    std::cout << "-= operator: now points to " << new_ptr->value << std::endl;
    
    // Pre-increment: ++ptr (returns new value)
    new_ptr = ++atomic_ptr;
    assert(new_ptr == &some_array[3]);
    std::cout << "Pre-increment: now points to " << new_ptr->value << std::endl;
    
    // Post-increment: ptr++ (returns old value)
    old_value = atomic_ptr++;
    assert(old_value == &some_array[3]);      // Returns old position
    assert(atomic_ptr.load() == &some_array[4]); // Pointer moved forward
    std::cout << "Post-increment: old was " << old_value->value 
              << ", now points to " << atomic_ptr.load()->value << std::endl;
    
    // Pre-decrement: --ptr (returns new value)
    new_ptr = --atomic_ptr;
    assert(new_ptr == &some_array[3]);
    std::cout << "Pre-decrement: now points to " << new_ptr->value << std::endl;
    
    // Post-decrement: ptr-- (returns old value)
    old_value = atomic_ptr--;
    assert(old_value == &some_array[3]);      // Returns old position
    assert(atomic_ptr.load() == &some_array[2]); // Pointer moved back
    std::cout << "Post-decrement: old was " << old_value->value 
              << ", now points to " << atomic_ptr.load()->value << std::endl;
    
    // Memory ordering can be specified for fetch_add/fetch_sub
    atomic_ptr.fetch_add(1, std::memory_order_release);
    
    // Operator forms always use memory_order_seq_cst (no way to specify otherwise)
    // For custom memory ordering, use fetch_add/fetch_sub functions
    
    // All return values are plain T* (not references to atomic<T*>)
    // This allows code to act on previous values without additional loads
    
    // Lock-free check (same as other atomic types)
    if (atomic_ptr.is_lock_free()) {
        std::cout << "atomic<T*> is lock-free on this platform" << std::endl;
    }
    
    return 0;
}