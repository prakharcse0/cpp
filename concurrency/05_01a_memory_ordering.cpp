#include <atomic>
#include <thread>
#include <iostream>
#include <vector>

// Memory Ordering: Controls how memory operations are reordered by CPU/compiler
// Problem: CPUs and compilers reorder instructions for performance
// Solution: Memory ordering constraints prevent dangerous reorderings

// THE FUNDAMENTAL PROBLEM:
// Without constraints, this could happen:
int x = 0, y = 0;
std::atomic<bool> ready(false);

void writer_thread() {
    x = 42;           // 1. Write to x
    y = 17;           // 2. Write to y  
    ready = true;     // 3. Signal ready
    // CPU might reorder to: ready=true, x=42, y=17 (BAD!)
}

void reader_thread() {
    while (!ready);   // Wait for ready
    // Might see ready=true but x=0, y=0 due to reordering!
    std::cout << x << ", " << y << std::endl;
}

// ============================================================================
// MEMORY ORDERING TYPES
// ============================================================================

void demonstrate_memory_orderings() {
    std::atomic<int> data(0);
    std::atomic<bool> flag(false);
    
    // 1. RELAXED ORDERING (memory_order_relaxed)
    // - No synchronization constraints
    // - Only guarantees atomicity of the single operation
    // - Allows maximum reordering
    // - Cheapest but most dangerous
    
    std::thread t1([&]() {
        data.store(42, std::memory_order_relaxed);  // Can be reordered
        flag.store(true, std::memory_order_relaxed); // Can be reordered
    });
    
    std::thread t2([&]() {
        while (!flag.load(std::memory_order_relaxed));
        // Might see flag=true but data=0! No guarantee about data visibility
        int value = data.load(std::memory_order_relaxed);
    });
    
    t1.join(); t2.join();
    
    // 2. ACQUIRE ORDERING (memory_order_acquire) - for LOADS
    // - No memory operations can be reordered BEFORE this load
    // - Creates a synchronization point for incoming data
    // - Used by consumer threads
    flag.store(false); data.store(0);
    
    std::thread t3([&]() {
        data.store(42, std::memory_order_relaxed);
        flag.store(true, std::memory_order_release); // Must pair with acquire
    });
    
    std::thread t4([&]() {
        while (!flag.load(std::memory_order_acquire)); // ACQUIRE barrier
        // Everything after this point sees all writes before the release
        int value = data.load(std::memory_order_relaxed); // Guaranteed to see 42
    });
    
    t3.join(); t4.join();
    
    // 3. RELEASE ORDERING (memory_order_release) - for STORES  
    // - No memory operations can be reordered AFTER this store
    // - Creates a synchronization point for outgoing data
    // - Used by producer threads
    // (Demonstrated above in t3)
    
    // 4. ACQ_REL ORDERING (memory_order_acq_rel) - for READ-MODIFY-WRITE
    // - Combines acquire and release
    // - No reordering before OR after the operation
    // - Used for operations that both read and write
    
    std::atomic<int> counter(0);
    counter.fetch_add(1, std::memory_order_acq_rel); // Both acquire and release
    
    // 5. SEQUENTIAL CONSISTENCY (memory_order_seq_cst) - DEFAULT
    // - Strongest ordering
    // - All operations appear to happen in some single global order
    // - Most expensive but safest
    // - This is the default when no ordering is specified
    
    flag.store(true);  // Same as memory_order_seq_cst
    flag.store(true, std::memory_order_seq_cst); // Explicit
}

// ============================================================================
// PRACTICAL EXAMPLES
// ============================================================================

void producer_consumer_example() {
    // Classic producer-consumer with proper memory ordering
    int shared_data = 0;
    std::atomic<bool> data_ready(false);
    
    // Producer thread
    std::thread producer([&]() {
        shared_data = 42;  // 1. Prepare data
        // RELEASE: Ensures shared_data write completes before flag is set
        data_ready.store(true, std::memory_order_release);
    });
    
    // Consumer thread  
    std::thread consumer([&]() {
        // ACQUIRE: Ensures we see all writes that happened before the release
        while (!data_ready.load(std::memory_order_acquire));
        // Guaranteed to see shared_data = 42
        std::cout << "Received data: " << shared_data << std::endl;
    });
    
    producer.join();
    consumer.join();
}

void compare_exchange_memory_ordering() {
    std::atomic<int> value(0);
    int expected = 0;
    
    // Compare-exchange can have DIFFERENT orderings for success/failure
    bool success = value.compare_exchange_weak(
        expected, 42,
        std::memory_order_acq_rel,    // SUCCESS: both acquire and release
        std::memory_order_acquire     // FAILURE: only acquire (no store occurred)
    );
    
    // Rules for failure ordering:
    // 1. Cannot be memory_order_release or memory_order_acq_rel (no store)
    // 2. Cannot be stronger than success ordering
    // 3. If not specified, defaults to success ordering with release stripped:
    //    - memory_order_release -> memory_order_relaxed
    //    - memory_order_acq_rel -> memory_order_acquire
    
    // These two calls are equivalent:
    value.compare_exchange_weak(expected, 42, std::memory_order_acq_rel);
    value.compare_exchange_weak(expected, 42, 
                               std::memory_order_acq_rel, 
                               std::memory_order_acquire);
}

void fence_synchronization() {
    // Memory fences provide synchronization without atomic operations
    int data1 = 0, data2 = 0;
    std::atomic<bool> flag(false);
    
    std::thread writer([&]() {
        data1 = 1;
        data2 = 2;
        std::atomic_thread_fence(std::memory_order_release); // Release fence
        flag.store(true, std::memory_order_relaxed);
    });
    
    std::thread reader([&]() {
        while (!flag.load(std::memory_order_relaxed));
        std::atomic_thread_fence(std::memory_order_acquire); // Acquire fence
        // Guaranteed to see data1=1, data2=2
        std::cout << data1 << ", " << data2 << std::endl;
    });
    
    writer.join();
    reader.join();
}

// ============================================================================
// PERFORMANCE CONSIDERATIONS
// ============================================================================

void performance_comparison() {
    std::atomic<int> counter(0);
    const int iterations = 1000000;
    
    // Relaxed: Fastest, no synchronization
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
    auto relaxed_time = std::chrono::high_resolution_clock::now() - start;
    
    // Seq_cst: Slowest, full synchronization
    counter.store(0);
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        counter.fetch_add(1, std::memory_order_seq_cst);
    }
    auto seq_cst_time = std::chrono::high_resolution_clock::now() - start;
    
    std::cout << "Relaxed: " << relaxed_time.count() << "ns\n";
    std::cout << "Seq_cst: " << seq_cst_time.count() << "ns\n";
}

// ============================================================================
// COMMON PATTERNS
// ============================================================================

// Pattern 1: Simple flag (use seq_cst for simplicity)
class SimpleFlag {
    std::atomic<bool> flag{false};
public:
    void set() { flag = true; }  // Default seq_cst
    bool is_set() { return flag; }
};

// Pattern 2: Producer-consumer (use acquire-release)
template<typename T>
class SingleProducerSingleConsumer {
    T data;
    std::atomic<bool> ready{false};
public:
    void produce(T value) {
        data = value;
        ready.store(true, std::memory_order_release);
    }
    
    T consume() {
        while (!ready.load(std::memory_order_acquire));
        return data; // Guaranteed to see the produced value
    }
};

// Pattern 3: Reference counting (use acq_rel for modification)
class RefCountedObject {
    mutable std::atomic<int> ref_count{1};
public:
    void add_ref() const {
        ref_count.fetch_add(1, std::memory_order_relaxed); // Just increment
    }
    
    void release() const {
        if (ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            // Last reference - need full synchronization before destruction
            delete this;
        }
    }
};

int main() {
    std::cout << "Memory ordering demonstrations:\n\n";
    
    demonstrate_memory_orderings();
    producer_consumer_example();
    compare_exchange_memory_ordering();
    fence_synchronization();
    
    std::cout << "\nKey takeaways:\n";
    std::cout << "- relaxed: Fast, no sync, only use for simple counters\n";
    std::cout << "- acquire: For consumers (loads)\n"; 
    std::cout << "- release: For producers (stores)\n";
    std::cout << "- acq_rel: For read-modify-write operations\n";
    std::cout << "- seq_cst: Default, safest, most expensive\n";
    
    return 0;
}

/*
MEMORY ORDERING QUICK REFERENCE:

Operation Type      | Common Orderings
--------------------|------------------------------------------
Load (read)         | relaxed, acquire, seq_cst
Store (write)       | relaxed, release, seq_cst  
Read-Modify-Write   | relaxed, acquire, release, acq_rel, seq_cst

Synchronization Pairs:
- release (store) ↔ acquire (load)
- acq_rel (RMW) ↔ acquire (load)
- seq_cst ↔ seq_cst (global ordering)

Rule of thumb:
1. Start with seq_cst (default) - it's always correct
2. Optimize to acquire-release for producer-consumer
3. Use relaxed only for simple counters where order doesn't matter
4. Never use relaxed for synchronization between different data
*/