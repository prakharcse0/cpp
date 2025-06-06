#include <iostream>
#include <atomic>
#include <memory>
#include <thread>

// Sample data structure for shared_ptr examples
struct MyData {
    int value;
    std::string name;
    
    MyData(int v, const std::string& n) : value(v), name(n) {}
};

// Global shared_ptr for atomic operations demo
std::shared_ptr<MyData> global_data_ptr;

int main() {
    // ===== BASIC FREE FUNCTIONS vs MEMBER FUNCTIONS =====
    
    std::atomic<int> atomic_int{42};
    
    // Member function style
    int value1 = atomic_int.load();
    atomic_int.store(100);
    bool is_lockfree1 = atomic_int.is_lock_free();
    
    // Equivalent free function style (takes pointer as first parameter)
    int value2 = std::atomic_load(&atomic_int);
    std::atomic_store(&atomic_int, 200);
    bool is_lockfree2 = std::atomic_is_lock_free(&atomic_int);
    
    std::cout << "Member function value: " << value1 << std::endl;
    std::cout << "Free function value: " << value2 << std::endl;
    std::cout << "Both lock-free checks equal: " << (is_lockfree1 == is_lockfree2) << std::endl;
    
    // ===== MEMORY ORDERING WITH FREE FUNCTIONS =====
    
    std::atomic<int> atomic_counter{0};
    
    // Free functions without explicit memory ordering (uses seq_cst default)
    std::atomic_store(&atomic_counter, 500);
    int loaded_value = std::atomic_load(&atomic_counter);
    
    // Free functions with explicit memory ordering (_explicit suffix)
    std::atomic_store_explicit(&atomic_counter, 600, std::memory_order_release);
    int loaded_explicit = std::atomic_load_explicit(&atomic_counter, std::memory_order_acquire);
    
    std::cout << "Loaded with default ordering: " << loaded_value << std::endl;
    std::cout << "Loaded with explicit ordering: " << loaded_explicit << std::endl;
    
    // ===== EXCHANGE OPERATIONS =====
    
    std::atomic<int> exchange_test{1000};
    
    // Member function style
    int old_value1 = exchange_test.exchange(2000);
    
    // Free function style
    int old_value2 = std::atomic_exchange(&exchange_test, 3000);
    
    // With explicit memory ordering
    int old_value3 = std::atomic_exchange_explicit(&exchange_test, 4000, std::memory_order_acq_rel);
    
    std::cout << "Exchange results: " << old_value1 << ", " << old_value2 << ", " << old_value3 << std::endl;
    
    // ===== COMPARE-EXCHANGE OPERATIONS =====
    
    std::atomic<int> cas_test{5000};
    
    // Member function - expected is a reference
    int expected_member = 5000;
    bool success1 = cas_test.compare_exchange_weak(expected_member, 6000);
    
    // Free function - expected is a pointer (C-compatible)
    int expected_free = 6000;
    bool success2 = std::atomic_compare_exchange_weak(&cas_test, &expected_free, 7000);
    
    // Free function with explicit memory ordering (requires both success and failure orders)
    int expected_explicit = 7000;
    bool success3 = std::atomic_compare_exchange_weak_explicit(
        &cas_test, 
        &expected_explicit, 
        8000,
        std::memory_order_release,    // success order
        std::memory_order_relaxed     // failure order
    );
    
    std::cout << "CAS results: " << success1 << ", " << success2 << ", " << success3 << std::endl;
    
    // ===== ATOMIC_FLAG OPERATIONS =====
    
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    
    // Member functions
    bool was_set1 = flag.test_and_set();
    flag.clear();
    
    // Free functions (note the "flag" in the name)
    bool was_set2 = std::atomic_flag_test_and_set(&flag);
    std::atomic_flag_clear(&flag);
    
    // With explicit memory ordering
    bool was_set3 = std::atomic_flag_test_and_set_explicit(&flag, std::memory_order_acquire);
    std::atomic_flag_clear_explicit(&flag, std::memory_order_release);
    
    std::cout << "Flag operations: " << was_set1 << ", " << was_set2 << ", " << was_set3 << std::endl;
    
    // ===== SHARED_PTR ATOMIC OPERATIONS =====
    
    // Initialize global shared_ptr
    global_data_ptr = std::make_shared<MyData>(42, "initial");
    
    // Atomic load operation on shared_ptr
    std::shared_ptr<MyData> local_copy = std::atomic_load(&global_data_ptr);
    std::cout << "Loaded shared_ptr data: " << local_copy->value << ", " << local_copy->name << std::endl;
    
    // Atomic store operation on shared_ptr
    auto new_data = std::make_shared<MyData>(99, "updated");
    std::atomic_store(&global_data_ptr, new_data);
    
    // Atomic exchange on shared_ptr
    auto another_data = std::make_shared<MyData>(123, "exchanged");
    auto old_data = std::atomic_exchange(&global_data_ptr, another_data);
    std::cout << "Exchanged data: " << old_data->value << ", " << old_data->name << std::endl;
    
    // Compare-exchange on shared_ptr
    auto expected_shared = another_data;
    auto desired_shared = std::make_shared<MyData>(456, "final");
    bool shared_cas_success = std::atomic_compare_exchange_weak(&global_data_ptr, &expected_shared, desired_shared);
    std::cout << "Shared_ptr CAS success: " << shared_cas_success << std::endl;
    
    // Check if shared_ptr operations are lock-free
    bool shared_ptr_lockfree = std::atomic_is_lock_free(&global_data_ptr);
    std::cout << "Shared_ptr operations lock-free: " << shared_ptr_lockfree << std::endl;
    
    // ===== EXPLICIT MEMORY ORDERING VARIANTS =====
    
    std::atomic<int> ordering_test{0};
    
    // All free functions have _explicit variants for custom memory ordering
    std::atomic_store_explicit(&ordering_test, 111, std::memory_order_relaxed);
    std::atomic_load_explicit(&ordering_test, std::memory_order_consume);
    
    // Shared_ptr also has explicit variants
    std::atomic_store_explicit(&global_data_ptr, new_data, std::memory_order_release);
    auto loaded_shared = std::atomic_load_explicit(&global_data_ptr, std::memory_order_acquire);
    
    // ===== C COMPATIBILITY DESIGN =====
    
    std::cout << "\n=== C Compatibility Features ===" << std::endl;
    std::cout << "1. All free functions use pointers (not references)" << std::endl;
    std::cout << "2. Naming convention: atomic_ prefix" << std::endl;
    std::cout << "3. _explicit suffix for memory ordering variants" << std::endl;
    std::cout << "4. Compatible with C atomic operations" << std::endl;
    
    // ===== FUNCTION NAMING PATTERNS =====
    
    std::cout << "\n=== Function Naming Patterns ===" << std::endl;
    std::cout << "Basic: std::atomic_load(&obj)" << std::endl;
    std::cout << "Explicit: std::atomic_load_explicit(&obj, order)" << std::endl;
    std::cout << "Flag: std::atomic_flag_test_and_set(&flag)" << std::endl;
    std::cout << "Flag explicit: std::atomic_flag_test_and_set_explicit(&flag, order)" << std::endl;
    
    // ===== PRACTICAL USAGE EXAMPLES =====
    
    // Simulate producer-consumer with free functions
    auto producer = [](std::atomic<int>* counter) {
        for (int i = 0; i < 5; ++i) {
            std::atomic_store_explicit(counter, i, std::memory_order_release);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    };
    
    auto consumer = [](std::atomic<int>* counter) {
        int last_seen = -1;
        for (int i = 0; i < 5; ++i) {
            int current;
            do {
                current = std::atomic_load_explicit(counter, std::memory_order_acquire);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            } while (current == last_seen);
            last_seen = current;
            std::cout << "Consumer saw: " << current << std::endl;
        }
    };
    
    std::atomic<int> shared_counter{-1};
    
    std::thread prod_thread(producer, &shared_counter);
    std::thread cons_thread(consumer, &shared_counter);
    
    prod_thread.join();
    cons_thread.join();
    
    std::cout << "\n=== Key Differences Summary ===" << std::endl;
    std::cout << "Member: obj.load() vs Free: atomic_load(&obj)" << std::endl;
    std::cout << "Member: obj.load(order) vs Free: atomic_load_explicit(&obj, order)" << std::endl;
    std::cout << "CAS Member: expected is reference vs Free: expected is pointer" << std::endl;
    std::cout << "Shared_ptr: Special case - not atomic type but has atomic operations" << std::endl;
    
    return 0;
}