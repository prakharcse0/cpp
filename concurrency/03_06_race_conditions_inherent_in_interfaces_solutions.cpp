#include <iostream>   // For std::cout, std::cerr
#include <thread>     // For std::thread, std::this_thread
#include <mutex>      // For std::mutex, std::lock_guard
#include <deque>      // Default container for std::stack (internal)
#include <stdexcept>  // For std::exception, std::logic_error
#include <memory>     // For std::shared_ptr, std::make_shared
#include <string>     // For std::string, std::to_string
#include <chrono>     // For std::this_thread::sleep_for
#include <utility>    // For std::move

// --- THEORY: Threading Pitfalls & Solutions ---

// 1. Data Leakage (Briefly covered as prerequisite):
//    - Problem: Passing pointers/references to protected data *outside* the mutex's scope.
//      This includes returning them, storing them globally, or passing to untrusted callbacks.
//    - Result: Bypasses mutex protection, leading to race conditions & undefined behavior.
//    - Guideline: DON'T pass pointers/references to protected data outside the lock's scope.
//      Instead, pass copies, value types, or expose well-defined operations that remain locked.

// 2. Race Conditions Inherent in Interfaces (Main Problem Focus):
//    - Problem: Even if individual operations (like push/pop/empty) are mutex-protected,
//      a *sequence* of operations (e.g., checking empty() then calling top()) can still be
//      subject to race conditions. The information becomes stale between calls.
//    - Example: `if (!s.empty()) { s.top(); s.pop(); }` - Another thread could pop
//      the last element *between* `empty()` and `top()`, leading to UB/crash.
//    - This is an interface design flaw, not a mutex implementation flaw.

// 2.1. The std::vector Copy Problem & Data Loss in pop() (if returning by value):
//    - Scenario: `pop()` designed to `remove` and `return value` (e.g., `std::vector<int> pop()`).
//    - Issue: `std::vector`'s copy constructor allocates heap memory. If memory allocation
//      fails during the copy (e.g., `std::bad_alloc`), and the item was *already removed*
//      from the stack's internal storage, the item is **permanently lost**.
//    - std::stack's original solution: `top()` (returns reference, no removal) and `pop()` (removes, no return).
//      This prevents data loss if `top()`'s copy fails (item stays on stack).
//    - Concurrency Dilemma: This `top()/pop()` split, while good for exception safety,
//      is the *source* of interface race conditions in multi-threaded code.
//      Combining them for concurrency reintroduces the data loss problem if return-by-value can throw.

// 3. Locking Granularity: The scope of data protected by a single mutex.
//    - Too Small: Leads to inherent interface races (e.g., protecting individual node links in a list).
//    - Too Large: (e.g., a single global mutex) Serializes all concurrent access, eliminating performance gains.
//    - Ideal: Just enough to protect invariants for complete logical operations, allowing maximum concurrency.

// 4. Deadlock:
//    - Problem: Occurs when two or more threads are waiting indefinitely for each other
//      to release a resource (mutex) that the other thread holds.
//    - Often arises when an operation requires locking *multiple* mutexes.
//    - Example: Thread A holds M1, tries to acquire M2. Thread B holds M2, tries to acquire M1. Both block.
//    - Contrast to Race Condition: Threads are stuck, not racing.

// --- END THEORY ---


// --- Custom Exception for Empty Stack ---
struct empty_stack : std::exception {
    const char* what() const noexcept override {
        return "empty stack";
    }
};

// --- Thread-Safe Stack Implementation (Solutions to Interface Races) ---
template<typename T>
class threadsafe_stack {
private:
    std::deque<T> data;      // Internal container
    mutable std::mutex m;   // Mutex protecting 'data'

public:
    // Default constructor
    threadsafe_stack() = default;

    // Copy Constructor: Thread-safe copy of the stack.
    // Locks the source object's mutex before copying its internal data.
    threadsafe_stack(const threadsafe_stack& other) {
        std::lock_guard<std::mutex> lock(other.m); // Acquire lock on 'other'
        data = other.data;                         // Perform copy while locked
    }

    // Assignment operator deleted: Prevents problematic copy assignment.
    // Thread ownership is unique, and complex move/copy logic for assignment
    // is often error-prone and undesirable for thread-safe containers.
    threadsafe_stack& operator=(const threadsafe_stack&) = delete;

    // --- Solution Option 1: Pass a reference to receive the popped value ---
    // Pros: Avoids return-by-value exception safety issues for T.
    // Cons: Requires caller to construct T instance, T must be assignable.
    void pop(T& value) {
        std::lock_guard<std::mutex> lock(m);
        if (data.empty()) {
            throw empty_stack();
        }
        value = data.front();     // Assign to caller's variable
        data.pop_front();         // Remove from internal deque
    }

    // --- Solution Option 3: Return a std::shared_ptr to the popped item ---
    // Pros: Exception-safe return (pointer copy is noexcept).
    //       Automatic memory management (shared_ptr).
    // Cons: Overhead for small types; caller gets a shared_ptr, not raw value.
    // How it helps: Ensures element is removed *only after* safely copied into shared_ptr.
    // If T's copy (into shared_ptr) throws, stack is untouched. shared_ptr return itself is noexcept.
    std::shared_ptr<T> pop() {
        std::lock_guard<std::mutex> lock(m); // 1. Acquire lock.
        if (data.empty()) {
            throw empty_stack();
        }
        // 2. Safely copy value into a new std::shared_ptr *while locked*.
        //    If this copy (e.g., T's copy constructor) throws, the stack is UNTOUCHED.
        std::shared_ptr<T> res(std::make_shared<T>(data.front()));
        // 3. ONLY THEN, remove the original from the stack.
        data.pop_front();
        // 4. Return the shared_ptr. Its own copy/move is *guaranteed not to throw*.
        return res;
    }
    // Note on Options 2 & 4: Option 2 (nothrow copy/move) is restrictive. Option 4 (both 1 & 2/3)
    // is implemented here by providing both `pop(T&)` and `pop() -> shared_ptr<T>`.

    // Push operation (protected write)
    void push(T new_value) {
        std::lock_guard<std::mutex> lock(m);
        data.push_front(std::move(new_value)); // Use std::move for efficiency
    }

    // Empty check (protected read)
    // While protected, results can be stale immediately after return.
    bool empty() const {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};

// --- Main Function (Demonstration) ---
int main() {
    std::cout << "--- Thread-Safe Stack Demonstration ---" << std::endl;

    threadsafe_stack<int> s;

    // Populate stack
    s.push(10);
    s.push(20);
    s.push(30);
    std::cout << "Stack initial empty: " << std::boolalpha << s.empty() << std::endl; // Should be false

    // Demonstrate pop by shared_ptr
    try {
        std::shared_ptr<int> val_ptr = s.pop();
        std::cout << "Popped via shared_ptr: " << *val_ptr << std::endl; // Should be 30
    } catch (const empty_stack& e) {
        std::cerr << "Error popping (shared_ptr): " << e.what() << std::endl;
    }

    // Demonstrate pop by reference
    try {
        int val_ref;
        s.pop(val_ref);
        std::cout << "Popped via reference: " << val_ref << std::endl; // Should be 20
    } catch (const empty_stack& e) {
        std::cerr << "Error popping (reference): " << e.what() << std::endl;
    }

    // Demonstrate empty stack check and exception handling for attempts to pop from empty
    try {
        s.pop(); // Pop remaining 10
        std::cout << "Popped last element." << std::endl;
        if (s.empty()) {
            std::cout << "Stack is now empty." << std::endl;
        }
        s.pop(); // This will throw empty_stack!
    } catch (const empty_stack& e) {
        std::cerr << "Caught expected exception when popping empty stack: " << e.what() << std::endl;
    }


    // --- Illustrating Inherent Interface Race (Conceptual Example) ---
    // Even with a fully protected `threadsafe_stack`, the "check-then-act" pattern is still dangerous.
    // The `empty()` call result can become stale *immediately* after it returns, before `pop()` is called.
    std::cout << "\n--- Illustrating Inherent Interface Race (Conceptual) ---" << std::endl;
    threadsafe_stack<int> race_s;
    race_s.push(100);

    // Thread A: Checks if not empty, then tries to pop.
    std::thread t_racer_A([&race_s]() {
        if (!race_s.empty()) { // Check 1: Stack is not empty.
            std::cout << "[Racer A] Stack not empty, trying to pop." << std::endl;
            // A race window exists *here*. Another thread could pop the last element.
            std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simulate delay
            try {
                std::shared_ptr<int> val = race_s.pop(); // Check 2: Try to pop.
                std::cout << "[Racer A] Successfully popped: " << *val << std::endl;
            } catch (const empty_stack& e) {
                std::cerr << "[Racer A ERROR] Failed to pop: " << e.what() << " (Race occurred!)" << std::endl;
            }
        } else {
            std::cout << "[Racer A] Stack was empty." << std::endl;
        }
    });

    // Thread B: Pops an element concurrently.
    std::thread t_racer_B([&race_s]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Give Racer A a head start on `empty()`
        if (!race_s.empty()) {
            std::cout << "[Racer B] Concurrently popping an item." << std::endl;
            try {
                race_s.pop(); // This pop might cause Racer A to fail.
            } catch (const empty_stack& e) {
                std::cerr << "[Racer B ERROR] " << e.what() << std::endl;
            }
        }
    });

    t_racer_A.join();
    t_racer_B.join();

    std::cout << "Final stack empty: " << std::boolalpha << race_s.empty() << std::endl;
    std::cout << "--- End Interface Race Demo ---" << std::endl;

    std::cout << "\n--- Main thread finished ---" << std::endl;
    return 0;
}