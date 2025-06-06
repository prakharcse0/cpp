#include <iostream>
#include <atomic>
#include <vector>
#include <string>
#include <cstring>

// Example of a User-Defined Type (UDT) that WORKS with std::atomic<>
struct SimpleCounter {
    int value;
    char flag;
    
    // Must have trivial copy-assignment (compiler-generated is trivial)
    // No virtual functions, no virtual base classes
    // No user-defined copy assignment operator
};

// Example of a UDT that DOESN'T work with std::atomic<>
struct ComplexType {
    std::vector<int> data;  // Has non-trivial copy assignment
    virtual void doSomething() {}  // Virtual function - not allowed
};

// Another example that DOESN'T work
struct CustomAssignment {
    int value;
    // Custom copy assignment operator makes it non-trivial
    CustomAssignment& operator=(const CustomAssignment& other) {
        value = other.value + 1;  // Custom logic prevents memcpy usage
        return *this;
    }
};

// Example that works - bitwise equality comparable
struct BitwiseComparable {
    int a;
    float b;
    // No padding issues, all members are trivially comparable
    // memcmp() will work correctly for equality comparison
};

// Example that might cause issues with compare_exchange
struct PaddingIssues {
    char c;     // 1 byte
    // 3 bytes of padding here on most systems
    int i;      // 4 bytes
    // Padding bytes may contain garbage, causing memcmp issues
};

int main() {
    // ===== BASIC USAGE OF std::atomic<UDT> =====
    
    // This works - SimpleCounter meets all requirements
    std::atomic<SimpleCounter> atomic_counter;
    
    // Initialize
    SimpleCounter initial_value{42, 'A'};
    atomic_counter.store(initial_value);
    
    // Load current value
    SimpleCounter current = atomic_counter.load();
    std::cout << "Current counter: " << current.value << ", flag: " << current.flag << std::endl;
    
    // Exchange operation
    SimpleCounter new_value{100, 'B'};
    SimpleCounter old_value = atomic_counter.exchange(new_value);
    std::cout << "Old value was: " << old_value.value << ", flag: " << old_value.flag << std::endl;
    
    // ===== COMPARE-EXCHANGE OPERATIONS =====
    
    // Compare and exchange - the core atomic operation for UDTs
    SimpleCounter expected{100, 'B'};
    SimpleCounter desired{200, 'C'};
    
    // Strong version - only fails if values actually differ
    bool success = atomic_counter.compare_exchange_strong(expected, desired);
    if (success) {
        std::cout << "Successfully updated to: " << desired.value << std::endl;
    } else {
        std::cout << "Update failed, current value: " << expected.value << std::endl;
    }
    
    // ===== FLOATING POINT GOTCHAS =====
    
    std::atomic<float> atomic_float{3.14f};
    
    // This might fail even if values are mathematically equal
    // due to different bit representations (NaN, +0.0 vs -0.0, etc.)
    float expected_float = 3.14f;
    float new_float = 2.71f;
    
    // Demonstrate potential issue with floating point representation
    float positive_zero = +0.0f;
    float negative_zero = -0.0f;
    
    std::cout << "Positive zero == negative zero: " << (positive_zero == negative_zero) << std::endl;
    std::cout << "memcmp result: " << (memcmp(&positive_zero, &negative_zero, sizeof(float)) == 0) << std::endl;
    
    // ===== SIZE LIMITATIONS AND PLATFORM SUPPORT =====
    
    // Most platforms support atomic operations for types <= sizeof(void*)
    struct SmallType { char data[sizeof(void*)]; };  // Usually works
    std::atomic<SmallType> small_atomic;
    
    // Some platforms support double-word compare-and-swap (DWCAS)
    struct DoubleWordType { 
        void* ptr1; 
        void* ptr2; 
    };  // May work on platforms with DWCAS
    
    // Check if operations are lock-free (hardware atomic instructions)
    std::cout << "SimpleCounter is lock-free: " << atomic_counter.is_lock_free() << std::endl;
    std::cout << "float is lock-free: " << atomic_float.is_lock_free() << std::endl;
    
    // ===== AVAILABLE OPERATIONS FOR UDT =====
    
    // std::atomic<UDT> only supports these operations (same as std::atomic<bool>):
    // - load()
    // - store()  
    // - exchange()
    // - compare_exchange_weak()
    // - compare_exchange_strong()
    // - Assignment and conversion operators
    
    SimpleCounter direct_assign{999, 'Z'};
    atomic_counter = direct_assign;  // Assignment operator
    
    SimpleCounter converted = atomic_counter;  // Conversion operator (calls load())
    std::cout << "Final value: " << converted.value << std::endl;
    
    // ===== WHAT DOESN'T WORK =====
    
    // These won't compile:
    // std::atomic<std::vector<int>> vec_atomic;  // Complex type
    // std::atomic<ComplexType> complex_atomic;   // Virtual functions
    // std::atomic<CustomAssignment> custom_atomic;  // Non-trivial assignment
    
    // No arithmetic operations available for UDTs:
    // atomic_counter++;  // Won't compile
    // atomic_counter += something;  // Won't compile
    
    // ===== WHY THESE RESTRICTIONS EXIST =====
    
    std::cout << "\n=== Why these restrictions exist ===" << std::endl;
    std::cout << "1. Compiler can use memcpy() for assignment (no user code to run)" << std::endl;
    std::cout << "2. Can use memcmp() for comparison (bitwise equality)" << std::endl;
    std::cout << "3. Avoids passing protected data to user functions" << std::endl;
    std::cout << "4. Prevents deadlocks from long-running user operations" << std::endl;
    std::cout << "5. Enables lock-free implementations using raw byte operations" << std::endl;
    std::cout << "6. Single internal lock can be used for all atomic UDT operations" << std::endl;
    
    return 0;
}