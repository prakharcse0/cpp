#include <iostream>   // For std::cout
#include <thread>     // For std::thread, std::this_thread, std::thread::id
#include <map>        // For std::map (example of associative container)
#include <chrono>     // For std::this_thread::sleep_for
#include <functional> // For std::hash (needed for unordered_map, or potentially for context)


// --- THEORY: std::thread::id ---
// 1. Identification: std::thread::id is the type for unique thread identifiers.
//    - Get from std::thread object: `thread_obj.get_id()`
//    - Get for current thread: `std::this_thread::get_id()`
//    - Default-constructed `std::thread::id()` means "not any thread."
// 2. Comparison: `std::thread::id` objects can be copied and compared (`==`, `!=`, `<`, `>`, etc.).
//    - Equal IDs: Same thread OR both are "not any thread."
//    - Unequal IDs: Different threads OR one is "not any thread."
//    - Comparison operators provide a total ordering (useful for `std::map`, sorting).
// 3. Hashing: `std::hash<std::thread::id>` allows use in unordered containers (`std::unordered_map`).
// 4. Use Cases:
//    - Differentiating threads (e.g., master thread vs. workers).
//    - Associating data with specific threads (e.g., `std::map<std::thread::id, ...>`).
//    - Debugging and logging (output to `std::cout` is implementation-defined, but equal IDs print same).
// --- END THEORY ---


// Global variable to store the ID of the "master" thread
std::thread::id master_thread_id;

void do_master_thread_work() {}
void do_common_work() {}

// Example
// --- Core Algorithm Part (shared by master and worker threads) ---
void some_core_part_of_algorithm() {
    // Example: Check if the current thread is the master thread
    if (std::this_thread::get_id() == master_thread_id) {
        do_master_thread_work(); // Master thread performs unique work
    }
    do_common_work(); // All threads perform common work
}

// --- Main Function ---
int main() {
    std::thread::id current_thread_id = std::this_thread::get_id();
    master_thread_id = current_thread_id;
    std::cout << "Main thread ID (master): " << master_thread_id << std::endl;
    // Main thread ID (master): 126689751902016

    std::thread t2;
    std::cout <<"Thread t2 id: " <<t2.get_id() <<std::endl;
    // Thread t2 id: thread::id of a non-executing thread
    
    std::thread t3{do_common_work};
    auto t3_id = t3.get_id();
    std::cout <<"Thread t3 id: " <<t3_id <<std::endl;
    // Thread t3 id: 126689745041088

    std::thread t4 = std::move(t3);
    std::cout <<"After doing: std::thread t4 = std::move(t3)   " <<std::endl;
    std::cout <<"Thread t3's new id: " <<t3.get_id() <<std::endl;
    // Thread t3's new id: thread::id of a non-executing thread
    std::cout <<"Thread t4 id: " <<t4.get_id() <<std::endl;
    // Thread t4 id: 126689745041088
    std::cout << "Is t4_id == old_t3_id?  " << std::boolalpha << (t4.get_id() == t3_id) << std::endl;
    // Is t4_id == old_t3_id ?  true

    t4.join();

    return 0;
}