#include <iostream>
#include <atomic>
#include <thread>
#include <vector>

// ==============================================================================
// C++ MEMORY MODEL TUTORIAL
// This tutorial covers the fundamental concepts of C++ memory model for beginners
// ==============================================================================


// CONCEPT 1: OBJECTS AND MEMORY LOCATIONS
// ======================================
// In C++, ALL DATA consists of objects (not OOP objects, but memory regions)
// An object is simply "a region of storage" with type and lifetime
struct ExampleStruct {
    int normal_int;        // Takes 1 memory location
    char normal_char;      // Takes 1 memory location
    
    // Bit fields demonstration - showing memory location sharing concept
    unsigned int bf1 : 4;  // means "4 bits of an unsigned int"
    unsigned int bf2 : 4;  // (they're adjacent in declaration)
    // bf1 and bf2 will likely share 1 memory location
    
    unsigned int : 0;      // Zero-width unnamed bit field - forces alignment
    // This forces bf4 to start in a NEW memory location
    
    unsigned int bf4 : 4;  // This gets its own separate memory location due to : 0 above
    // unsigned int bf5 : 40;   // ERROR! 40 > 32 bits of unsigned int
    
    std::string s;         // Complex object - uses multiple internal memory locations
    
    // ACTUAL Memory Layout (with the : 0 separator):
    // Memory Location 1: [normal_int - 4 bytes]
    // Memory Location 2: [normal_char - 1 byte] [padding - 3 bytes]
    // Memory Location 3: [bf1:4bits][bf2:4bits][unused:24bits]  // bf1 & bf2 share
    // Memory Location 4: [bf4:4bits][unused:28bits]             // bf4 gets own location
    // Memory Location 5+: [std::string internal data - multiple locations]
    
    // Why 3 Bytes of Padding After normal_char?
    // Alignment Requirements:
    // int needs 4-byte alignment (address divisible by 4)
    // char needs 1-byte alignment
    // unsigned int (for bit fields) needs 4-byte alignment
    
    /*
    Why This Matters for Concurrency:
    
    Safe concurrent access:
    // Thread 1 can safely modify normal_int
    thread1: my_struct.normal_int = 42;
    
    // Thread 2 can safely modify normal_char  
    thread2: my_struct.normal_char = 'A';
    // No data race - different memory locations!
    
    // Thread 3 can safely modify bf4
    thread3: my_struct.bf4 = 3;
    // No data race with bf1/bf2 - bf4 is in separate location due to : 0
    
    Dangerous concurrent access:
    // Thread 1 modifies bf1
    thread1: my_struct.bf1 = 5;
    
    // Thread 2 modifies bf2
    thread2: my_struct.bf2 = 7;
    // POTENTIAL data race - bf1 and bf2 share the same memory location!
    // Both threads are modifying the same 4-byte int
    */
    
    /*
    The Zero-Length Bit Field Effect (which we DO have):
    unsigned int bf2 : 4;
    unsigned int : 0;        // Forces bf4 to new memory location
    unsigned int bf4 : 4;    // Now gets its own location
    
    This DOES force bf4 into a separate memory location, making concurrent 
    access to bf1/bf2 vs bf4 safe. This syntax IS widely supported.
    */
    
    /*
    What We Know For Sure With This Code:
    
    What the C++ Standard Guarantees:
    - bf1 and bf2 (adjacent, same type, no : 0 between) will share a location
    - The : 0 forces bf4 to start at a new storage unit boundary
    - bf4 will NOT share a location with bf1/bf2
    
    Actual Layout:
    Storage unit 1: [bf1:4][bf2:4][unused:24] - bf1 and bf2 together
    Storage unit 2: [bf4:4][unused:28]        - bf4 separate due to : 0
    
    This gives us predictable concurrent access patterns:
    - Modifying bf1 and bf2 simultaneously: UNSAFE (same storage)
    - Modifying bf1/bf2 vs bf4 simultaneously: SAFE (different storage)
    */
};


void demonstrate_objects_and_memory_locations() {
    // KEY RULES FROM C++ STANDARD:
    // 1. Every variable is an object (including members of other objects)
    // 2. Every object occupies at least one memory location
    // 3. Fundamental types (int, char, etc.) = exactly one memory location
    // 4. Adjacent bit fields = same memory location (unless separated by unnamed zero-length field)
    //    Note: Zero-length bit field separation is compiler-dependent and may not compile everywhere

    std::cout << "\n=== OBJECTS AND MEMORY LOCATIONS ===\n";
    
    // Every variable is an object
    int simple_var = 42;           // 1 object, 1 memory location
    int array[3] = {1, 2, 3};      // 1 array object with 3 subobjects, 3 memory locations
    ExampleStruct my_struct;       // 1 struct object with multiple subobjects
    
    std::cout << "Simple variable occupies " << sizeof(simple_var) << " bytes\n";
    std::cout << "Array occupies " << sizeof(array) << " bytes total\n";
    std::cout << "Struct occupies " << sizeof(my_struct) << " bytes total\n";

    std::cout <<"\nExampleStruct obj: " <<std::endl;
    ExampleStruct obj;    
    // Check addresses
    std::cout << "Address of normal_int: " << &obj.normal_int << std::endl;
    std::cout << "Address of normal_char: " << &(obj.normal_char) << std::endl;
    
    // Bit fields don't have individual addresses! This won't compile:
    // std::cout << &obj.bf1;  // ERROR: can't take address of bit field
    
    // But we can check if they're in the same storage unit:
    std::cout << "Size of struct: " << sizeof(ExampleStruct) << std::endl;
    std::cout << "Offset of normal_int: " << offsetof(ExampleStruct, normal_int) << std::endl;
    std::cout << "Offset of normal_char: " << offsetof(ExampleStruct, normal_char) << std::endl;
    // std::cout << "Offset of bf1: " << offsetof(ExampleStruct, bf1) << std::endl;
    // You cannot take the address of bit fields or use offsetof with them. 
    // This is because bit fields don't have individual addresses - they're packed into larger storage units.
    std::cout << "Offset of normal_char: " << offsetof(ExampleStruct, normal_char) << std::endl;
    std::cout << "Offset of normal_char: " << offsetof(ExampleStruct, normal_char) << std::endl;
    std::cout << "Offset of normal_char: " << offsetof(ExampleStruct, normal_char) << std::endl;

}


// CONCEPT 2: MEMORY LOCATIONS AND CONCURRENCY
// ===========================================
// The memory location concept is CRUCIAL for thread safety

// ATOMIC KEYWORD EXPLANATION:
// std::atomic<T> makes operations on type T indivisible and thread-safe
// It's like putting a protective wrapper around your variable

std::atomic<int> safe_counter{0};  // Atomic operations on this are safe
int unsafe_counter = 0;            // Non-atomic operations create data races

void safe_increment() {
   // This is an atomic operation - indivisible, thread-safe
   safe_counter.fetch_add(1);
}

void unsafe_increment() {
   // This is NOT atomic - can cause data race and undefined behavior
   // The operation involves: load, increment, store - can be interrupted
   unsafe_counter++;
}

void demonstrate_concurrency_safety() {
   // CONCURRENCY RULES FOR MEMORY LOCATIONS:
   // 1. Different memory locations + different threads = ALWAYS SAFE
   // 2. Same memory location + multiple readers = SAFE (read-only data)
   // 3. Same memory location + any writer + any other access = POTENTIAL DATA RACE
   //
   // DATA RACE DEFINITION:
   // - Two+ threads access same memory location
   // - At least one access is a write  
   // - At least one access is non-atomic
   // - No enforced ordering between accesses
   // Result: UNDEFINED BEHAVIOR (program can do literally anything)
   //
   // SOLUTIONS TO AVOID DATA RACES:
   // 1. Use mutexes to enforce ordering (serialize access)
   // 2. Use atomic operations (brings back defined behavior)
   // Note: Atomic ops don't prevent race condition, just data race
   //
   // ORDERING ENFORCEMENT:
   // - Each PAIR of accesses needs defined ordering (if >2 threads involved)
   // - Mutex: only one thread accesses at a time (clear ordering)
   // - Atomic: synchronization properties enforce ordering
   
   std::cout << "\n=== CONCURRENCY AND MEMORY LOCATIONS ===\n";
   
   // SAFE: Different threads accessing SEPARATE memory locations
   int thread1_var = 1;  // Memory location A
   int thread2_var = 2;  // Memory location B
   // No problem - threads work on different locations
   
   // SAFE: Multiple threads READING the same memory location
   const int shared_readonly = 100;
   // Multiple threads can safely read this without synchronization
   
   // DANGEROUS: Multiple threads accessing SAME memory location with writes
   // This creates a DATA RACE and UNDEFINED BEHAVIOR
   
   std::cout << "Safe: different memory locations or read-only access\n";
   std::cout << "Dangerous: same location + writes without synchronization\n";
   
   // Demonstration with threads using atomic operations (safe)
   std::vector<std::thread> threads;
   
   for(int i = 0; i < 10; ++i) {
       threads.emplace_back(safe_increment);
   }
   
   for(auto& t : threads) {
       t.join();
   }
   
   std::cout << "Safe atomic counter result: " << safe_counter.load() << "\n";
   
    // Data race here, high chances :
    // struct Example {
    //     unsigned int bf1 : 4;  // bits 0-3 of the same int
    //     unsigned int bf2 : 4;  // bits 4-7 of the same int
    // };

    // // Memory layout (likely):
    // // [bf1: 4 bits][bf2: 4 bits][unused: 24 bits] - all in one 32-bit int
    // Why data race occurs:
    // When Thread 1 writes to bf1 and Thread 2 writes to bf2:

    // Thread 1: Load entire 32-bit int → Modify bf1's bits → Store entire 32-bit int
    // Thread 2: Load entire 32-bit int → Modify bf2's bits → Store entire 32-bit int

    // These operations can interleave:
    // Thread 1: Load int (value = 0x12)
    // Thread 2: Load int (value = 0x12)  // Same value!
    // Thread 1: Modify bf1, Store (value = 0x15)
    // Thread 2: Modify bf2, Store (value = 0x32)  // Overwrites Thread 1's change!
    // Result: Thread 1's write to bf1 gets lost because both threads are modifying the same memory location.

}

void demonstrate_atomic_operations() {
   // ATOMIC OPERATIONS ARE INDIVISIBLE:
   // - Either they complete fully, or they don't happen at all
   // - No other thread can see the operation "half-done"
   // - Multiple threads can safely call atomic operations simultaneously
   
   std::cout << "\nStarting atomic demonstration...\n";
   
   // Launch 10 threads, each incrementing 1000 times
   std::vector<std::thread> threads;
   
   // Reset counters
   safe_counter = 0;
   unsafe_counter = 0;
   
   // Create threads that increment safely
   for(int i = 0; i < 10; ++i) {
       threads.emplace_back([]() {
           for(int j = 0; j < 1000; ++j) {
               safe_increment();
           }
       });
   }
   
   // Create threads that increment unsafely  
   for(int i = 0; i < 10; ++i) {
       threads.emplace_back([]() {
           for(int j = 0; j < 1000; ++j) {
               unsafe_increment();
           }
       });
   }
   
   // Wait for all threads to finish
   for(auto& t : threads) {
       t.join();
   }
   
   std::cout << "Expected result: 10000\n";
   std::cout << "Atomic counter: " << safe_counter.load() << " (should be 10000)\n";
   std::cout << "Unsafe counter: " << unsafe_counter << " (probably NOT 10000!)\n";
   
   // COMMON ATOMIC OPERATIONS:
   std::atomic<int> demo_atomic{42};
   
   std::cout << "\nAtomic operations demo:\n";
   std::cout << "Initial value: " << demo_atomic.load() << "\n";
   
   // store() - atomically sets new value
   demo_atomic.store(100);
   std::cout << "After store(100): " << demo_atomic.load() << "\n";
   
   // fetch_add() - atomically adds and returns OLD value
   int old_val = demo_atomic.fetch_add(5);
   std::cout << "fetch_add(5) returned: " << old_val << ", new value: " << demo_atomic.load() << "\n";
   
   // exchange() - atomically sets new value and returns OLD value
   int previous = demo_atomic.exchange(200);
   std::cout << "exchange(200) returned: " << previous << ", new value: " << demo_atomic.load() << "\n";
   
   // compare_and_swap (compare_exchange_weak/strong)
   int expected = 200;
   bool success = demo_atomic.compare_exchange_strong(expected, 300);
   std::cout << "compare_exchange_strong(200->300): " << (success ? "success" : "failed") << "\n";
   std::cout << "Final value: " << demo_atomic.load() << "\n";
}

// ATOMIC WITH CUSTOM TYPES:
struct Point {
   int x, y;
   Point(int x = 0, int y = 0) : x(x), y(y) {}
};

void demonstrate_atomic_structs() {
   // You can make custom types atomic too (if they're trivially copyable)
   std::atomic<Point> atomic_point{Point{10, 20}};
   
   std::cout << "\nAtomic struct operations:\n";
   
   // Load the entire struct atomically
   Point current = atomic_point.load();
   std::cout << "Current point: (" << current.x << ", " << current.y << ")\n";
   
   // Store a new struct atomically
   atomic_point.store(Point{50, 60});
   current = atomic_point.load();
   std::cout << "After store: (" << current.x << ", " << current.y << ")\n";
   
   // Exchange operation
   Point old_point = atomic_point.exchange(Point{100, 200});
   std::cout << "Exchanged (" << old_point.x << ", " << old_point.y << ") -> (100, 200)\n";
}


// CONCEPT 3: MODIFICATION ORDERS
// =============================
// Every object has a defined modification order - sequence of all writes to it

std::atomic<int> shared_object{0};

void demonstrate_modification_orders() {
    // MODIFICATION ORDERS: The Timeline of Object Changes
    // - Every object has ONE modification order (sequence of ALL writes to it)
    // - Starts with object's initialization  
    // - Includes writes from ALL threads in the program
    // - Order may vary between different program runs
    // - BUT all threads MUST agree on the order within single execution
    //
    // YOUR RESPONSIBILITY with non-atomic objects:
    // - Ensure sufficient synchronization (mutexes, etc.)
    // - Make sure all threads see same modification order
    // - If threads see different sequences = data race = undefined behavior
    //
    // COMPILER'S RESPONSIBILITY with atomic objects:
    // - Ensures necessary synchronization automatically
    // - Prevents certain speculative execution optimizations
    // - Once thread sees value X, later reads must see X or newer values
    //
    // ORDERING CONSTRAINTS:
    // - Read after write in same thread: must see written value or later
    // - Threads agree on individual object orders, but NOT relative order between different objects
    
    std::cout << "\n=== MODIFICATION ORDERS ===\n";
    
    shared_object.store(10);  // Write #1 in modification order
    shared_object.store(20);  // Write #2 in modification order  
    shared_object.store(30);  // Write #3 in modification order
    
    // All threads will see: initial(0) -> 10 -> 20 -> 30
    // (though they might not observe all intermediate values)
    
    int current_value = shared_object.load();
    std::cout << "Current value: " << current_value << "\n";
    
    // WHY MODIFICATION ORDERS MATTER:
    // Without them, concurrent programming would be impossible - each thread
    // could see different "histories" of the same data (e.g., 30->10->20)
    // This would make program logic unpredictable and cause data corruption
    
    // VALID OBSERVATIONS (respecting modification order):
    // Thread A: 0 -> 30 (skipped intermediates) ✓
    // Thread B: 0 -> 10 -> 30 (missed 20) ✓
    // Thread C: 10 -> 20 -> 30 (started late) ✓
    
    // INVALID OBSERVATIONS (violating modification order):
    // Thread X: 30 -> 20 (going backwards) ❌
    // Thread Y: 20 -> 10 (going backwards) ❌
    // Key rule: Once you see a value, you can never see an older value
}

void demonstrate_atomic_vs_non_atomic() {
    std::cout << "\n=== ATOMIC vs NON-ATOMIC ===\n";
    
    // NON-ATOMIC: No modification order guarantees
    int regular_int = 0;
    // regular_int = 10; regular_int = 20;
    // Other threads might see: 20->10 or garbage values or partial writes ❌
    // YOU must provide synchronization (mutex, etc.)
    
    // ATOMIC: Automatic modification order guarantees  
    std::atomic<int> atomic_int{0};
    atomic_int.store(10);
    atomic_int.store(20);
    // Other threads guaranteed to see: 0->10->20 or 0->20 (never 20->10) ✓
    // COMPILER provides synchronization automatically
    
    std::cout << "Regular int: " << regular_int << " (needs manual sync)\n";
    std::cout << "Atomic int: " << atomic_int.load() << " (auto-synchronized)\n";
    
    // WHAT ATOMICS ACTUALLY DO:
    // 1. Hardware-level indivisible operations (all-or-nothing writes)
    // 2. Memory barriers prevent CPU/compiler reordering
    // 3. Cache coherency keeps all cores synchronized
    // Trade-off: Slower execution for guaranteed correctness
}


// CONCEPT 4: ATOMIC OPERATIONS
// ===========================
// Atomic operations are INDIVISIBLE - cannot be observed half-done

void demonstrate_atomic_operations() {
    // ATOMIC OPERATIONS: The Foundation of Thread-Safe Programming
    // - INDIVISIBLE: Cannot be observed half-done by any thread
    // - Either completely done or not done at all
    // - If load is atomic AND all modifications are atomic = consistent reads
    //
    // NON-ATOMIC OPERATIONS: The Source of Data Races  
    // - Can be seen "half-done" by other threads
    // - Store operation: other thread might see partial/corrupted value
    // - Load operation: might read part of old value + part of new value
    // - This creates data races and undefined behavior
    //
    // C++ REQUIREMENT: Use atomic types (std::atomic<T>) for atomic operations
    
    std::cout << "\n=== ATOMIC OPERATIONS ===\n";
    
    std::atomic<int> atomic_var{100};
    
    // ATOMIC OPERATIONS: Indivisible, thread-safe
    int value1 = atomic_var.load();           // Atomic load
    atomic_var.store(200);                    // Atomic store
    int value2 = atomic_var.exchange(300);    // Atomic exchange
    int value3 = atomic_var.fetch_add(50);    // Atomic fetch-and-add
    
    std::cout << "Loaded value: " << value1 << "\n";
    std::cout << "Exchanged value: " << value2 << "\n";
    std::cout << "Value before add: " << value3 << "\n";
    std::cout << "Final value: " << atomic_var.load() << "\n";
    
    // NON-ATOMIC EXAMPLE (dangerous in multithreaded code):
    int regular_var = 100;
    // regular_var++; // NOT atomic: load -> increment -> store (3 separate steps!)
    // Another thread might interrupt between any of these steps
}

// PUTTING IT ALL TOGETHER: Why This Matters
void demonstrate_why_this_matters() {
    // WHY MEMORY MODEL MATTERS:
    // - Data races cause undefined behavior (program can literally do ANYTHING)
    // - One documented case: undefined behavior caused monitor to catch fire!
    // - Memory corruption, crashes, security vulnerabilities all possible
    // - These bugs are extremely hard to debug and reproduce
    //
    // SOLUTIONS TO AVOID UNDEFINED BEHAVIOR:
    // 1. Use atomic operations for shared data (brings back defined behavior)
    // 2. Use mutexes to enforce ordering between thread accesses
    // 3. Ensure only one thread modifies data, others just read
    // 4. Access separate memory locations from different threads (always safe)
    //
    // REMEMBER: Atomic operations don't prevent race conditions entirely
    // - They prevent DATA races (which cause undefined behavior)
    // - But logic races (wrong order of operations) can still happen
    // - You still need proper synchronization design
}

int main() {
    std::cout << "C++ MEMORY MODEL TUTORIAL\n";
    std::cout << "========================\n";
    
    demonstrate_objects_and_memory_locations();
    demonstrate_concurrency_safety();
    demonstrate_atomic_operations();
    demonstrate_atomic_structs();
    demonstrate_modification_orders();
    demonstrate_atomic_operations();
    demonstrate_why_this_matters();
    
    // FINAL SUMMARY OF KEY CONCEPTS:
    // - All C++ data = objects occupying memory locations
    // - Same memory location + concurrent access + writes = need synchronization
    // - Every object has modification order (sequence of writes all threads agree on)
    // - Atomic operations are indivisible, prevent data races
    // - Data races cause undefined behavior (program can do literally anything)
    
    return 0;
}