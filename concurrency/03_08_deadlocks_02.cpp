// ============================================================================
// DEADLOCK PREVENTION GUIDELINES - COMPREHENSIVE NOTES
// ============================================================================

#include <iostream>
#include <mutex>
#include <thread>
#include <stdexcept>
#include <climits>
#include <atomic>
#include <condition_variable>
#include <vector>
#include <functional>

// ============================================================================
// CORE CONCEPT: Deadlock can occur with ANY synchronization construct
// Not just locks - threads waiting for each other can also deadlock
// ============================================================================

void deadlock_with_threads() {
    // BAD EXAMPLE: Producer-consumer deadlock without locks
    std::atomic<bool> t1_ready{false}, t2_ready{false};
    
    std::thread t1([&]() {
        std::cout << "T1: Waiting for T2 to be ready...\n";
        while (!t2_ready.load()) {
            std::this_thread::yield(); // T1 waits for T2 signal
        }
        std::cout << "T1: Done\n";
    });
    
    std::thread t2([&]() {
        std::cout << "T2: Waiting for T1 to be ready...\n";
        while (!t1_ready.load()) {
            std::this_thread::yield(); // T2 waits for T1 signal
        }
        std::cout << "T2: Done\n";
    });
    
    // DEADLOCK: T1 waits for t2_ready, T2 waits for t1_ready
    // Neither thread ever sets the flag the other is waiting for
    // This will spin forever - both threads stuck in while loops
    
    t1.join(); // Will hang - T1 never finishes
    t2.join(); // Will hang - T2 never finishes
}

void good_thread_coordination() {
    // GOOD: Clear hierarchy - main thread manages worker threads
    std::thread worker1([]() {
        std::cout << "Worker1: Doing work...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
    
    std::thread worker2([]() {
        std::cout << "Worker2: Doing work...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
    
    // Main thread waits for workers (one-way dependency)
    worker1.join();  // OK: Clear hierarchy
    worker2.join();  // OK: Clear hierarchy
}

void deadlock_with_condition_variables() {
    std::mutex mtx1, mtx2;
    std::condition_variable cv1, cv2;
    bool ready1 = false, ready2 = false;
    
    // BAD: Each thread waits for the other's condition
    std::thread t1([&]() {
        std::unique_lock<std::mutex> lock(mtx1);
        cv2.wait(lock, [&]{ return ready2; });  // Wait for T2's signal
        ready1 = true;
        cv1.notify_one();
    });
    
    std::thread t2([&]() {
        std::unique_lock<std::mutex> lock(mtx2);
        cv1.wait(lock, [&]{ return ready1; });  // Wait for T1's signal  
        ready2 = true;
        cv2.notify_one();
    });
    
    // DEADLOCK: Each waits for other's condition that never gets set
    t1.join();
    t2.join();
}

// ============================================================================
// GUIDELINE 1: AVOID NESTED LOCKS
// Don't acquire a lock if you already hold one
// ============================================================================

std::mutex mutex1, mutex2;

void bad_nested_locks() {
    std::lock_guard<std::mutex> lock1(mutex1);  // Hold first lock
    std::lock_guard<std::mutex> lock2(mutex2);  // BAD: Acquire second while holding first
    // Potential deadlock if another thread locks in opposite order
}

void good_multiple_locks() {
    // GOOD: Acquire multiple locks atomically using std::lock
    std::lock(mutex1, mutex2);
    std::lock_guard<std::mutex> lock1(mutex1, std::adopt_lock);
    std::lock_guard<std::mutex> lock2(mutex2, std::adopt_lock);
    // Both locks acquired without deadlock risk
}

// ============================================================================
// GUIDELINE 2: AVOID CALLING USER-SUPPLIED CODE WHILE HOLDING A LOCK
// User code might acquire locks, violating nested lock rule
// PROBLEM: When writing generic/library code, you don't know what user functions will do:
// - Acquire their own locks → nested lock violation
// - Call back into your code → reentrancy issues  
// - Do expensive operations → hold locks too long
// - Throw exceptions → lock state issues
// ============================================================================

template<typename T>
class ThreadSafeStack {
    std::mutex mtx;
    std::vector<T> data;
    
public:
    void push(T item) {
        std::lock_guard<std::mutex> lock(mtx);
        data.push_back(item);  // OK: Standard library code - predictable, no locks
    }
    
    void bad_operation(T item, std::function<void(T&)> user_func) {
        std::lock_guard<std::mutex> lock(mtx);  // We acquire our lock
        user_func(item);  // BAD: User function runs with our lock held!
        data.push_back(item);
        
        // DANGERS while lock is held:
        // 1. user_func might try to acquire another lock → deadlock risk
        // 2. user_func might call back into this stack → reentrancy
        // 3. user_func might be slow → poor performance  
        // 4. user_func might throw → exception safety issues
    }
    
    void better_operation(T item, std::function<void(T&)> user_func) {
        user_func(item);  // BETTER: Call user code first (no locks held)
        std::lock_guard<std::mutex> lock(mtx);  // Then acquire lock for our work
        data.push_back(item);
        
        // SAFER because:
        // 1. User code runs freely - can acquire any locks without deadlock
        // 2. No reentrancy issues - our lock not held during user code
        // 3. Lock held for minimal time - just for our data.push_back()
        // 4. Clear separation of concerns
    }
};

// CONCRETE EXAMPLE: Why better_operation avoids deadlock
ThreadSafeStack<int> global_stack1;
ThreadSafeStack<int> global_stack2;

void dangerous_user_callback(int& value) {
    std::cout << "User processing: " << value << std::endl;
    
    // User code interacts with other stacks - this is unpredictable!
    global_stack2.push(value * 2);  // Tries to acquire stack2's mutex
    
    // Even worse - reentrancy!
    // global_stack1.push(value + 1);  // Would try to acquire stack1's mutex AGAIN!
}

void demonstrate_deadlock_scenario() {
    std::thread t1([&]() {
        // BAD: Thread 1 holds stack1.mtx, then user code tries to get stack2.mtx
        global_stack1.bad_operation(10, dangerous_user_callback);
        // Timeline: Acquire stack1.mtx → user_callback() → tries stack2.mtx → DANGER!
    });
    
    std::thread t2([&]() {
        // Thread 2 might hold stack2.mtx and try to use stack1
        global_stack2.push(99);  // If this happens during t1's user_callback... DEADLOCK!
    });
    
    t1.join();
    t2.join();
}

void demonstrate_safe_approach() {
    std::thread t1([&]() {
        // GOOD: User code runs first (no locks held), then we do our work
        global_stack1.better_operation(10, dangerous_user_callback);
        // Timeline: user_callback() runs freely → then acquire stack1.mtx → safe!
    });
    
    std::thread t2([&]() {
        global_stack2.push(99);  // This can run anytime without conflict
    });
    
    t1.join();
    t2.join();
}

// TIMELINE COMPARISON:
// BAD:    [Acquire Lock] → [User Code with Lock Held] → [Release Lock]
//                              ↓ DANGER ZONE ↓
//                         User might acquire other locks
//
// BETTER: [User Code Runs Freely] → [Acquire Lock] → [Our Work] → [Release Lock]
//              ↓ SAFE ZONE ↓              ↓ MINIMAL LOCK TIME ↓
//         No locks held yet            Only our operation protected

// ============================================================================
// GUIDELINE 3: ACQUIRE LOCKS IN A FIXED ORDER
// If you must acquire multiple locks, do it in same order across all threads
// ============================================================================

class BankAccount {
    std::mutex mtx;
    int balance;
public:
    friend void bad_transfer(BankAccount& from, BankAccount& to, int amount);
    friend void good_transfer(BankAccount& from, BankAccount& to, int amount);    
};

void bad_transfer(BankAccount& from, BankAccount& to, int amount) {
    std::lock_guard<std::mutex> lock1(from.mtx);  // Order depends on parameter order
    std::lock_guard<std::mutex> lock2(to.mtx);    // Different threads might lock in reverse
    // DEADLOCK RISK: transfer(a,b) vs transfer(b,a) can deadlock
}

void good_transfer(BankAccount& from, BankAccount& to, int amount) {
    // GOOD: Always lock in consistent order (e.g., by memory address)
    BankAccount* first = &from < &to ? &from : &to;
    BankAccount* second = &from < &to ? &to : &from;
    
    std::lock_guard<std::mutex> lock1(first->mtx);
    std::lock_guard<std::mutex> lock2(second->mtx);
    // Always same order regardless of parameter order
}

// ============================================================================
// LINKED LIST EXAMPLE: Hand-over-hand locking
// Must traverse in consistent direction to avoid deadlock
// ============================================================================

struct ListNode {
    std::mutex mtx;
    int data;
    std::shared_ptr<ListNode> next;
};

class ThreadSafeList {
    std::shared_ptr<ListNode> head;
    
public:
    void traverse_forward() {
        auto current = head;
        if (!current) return;
        
        std::unique_lock<std::mutex> current_lock(current->mtx);
        
        while (current->next) {
            std::unique_lock<std::mutex> next_lock(current->next->mtx);
            current_lock.unlock();  // Release previous lock
            current = current->next;
            current_lock = std::move(next_lock);  // Hand-over-hand
        }
    }
    
    void bad_traverse_backward() {
        // BAD: If two threads traverse in opposite directions,
        // they can deadlock in the middle of the list
        // Thread 1: holds A, wants B
        // Thread 2: holds B, wants A
    }
    
    void delete_node_bad(std::shared_ptr<ListNode> node_b) {
        // BAD: Locking B before A and C can cause deadlock
        std::lock_guard<std::mutex> lock_b(node_b->mtx);
        // Now trying to lock adjacent nodes A and C...
        // But traversing thread might hold A or C and want B
    }
    
    void delete_node_good(std::shared_ptr<ListNode> prev, 
                         std::shared_ptr<ListNode> node, 
                         std::shared_ptr<ListNode> next) {
        // GOOD: Lock in consistent order (A before B before C)
        std::lock(prev->mtx, node->mtx, next->mtx);
        std::lock_guard<std::mutex> lock_prev(prev->mtx, std::adopt_lock);
        std::lock_guard<std::mutex> lock_node(node->mtx, std::adopt_lock);
        std::lock_guard<std::mutex> lock_next(next->mtx, std::adopt_lock);
    }
};

// ============================================================================
// GUIDELINE 4: USE A LOCK HIERARCHY
// CONCEPT: Assign numerical levels to mutexes, enforce "only lock DOWN the ladder"
// Rule: Current thread level MUST be > mutex level to acquire it
// WHY IT WORKS: Makes deadlock cycles impossible - no thread can want what 
//               another higher-level thread holds
// ============================================================================

class hierarchical_mutex {
    std::mutex internal_mutex;                    // The actual working mutex
    unsigned long const hierarchy_value;          // This mutex's level (immutable)
    unsigned long previous_hierarchy_value;       // Save for unlock restoration
    static thread_local unsigned long this_thread_hierarchy_value; // Each thread's current level
    
    void check_for_hierarchy_violation() {
        if (this_thread_hierarchy_value <= hierarchy_value) {
            throw std::logic_error("mutex hierarchy violated");
        }
        // Logic: Thread at level 10000 can lock level 5000 ✅ (10000 > 5000)
        //        Thread at level 5000 cannot lock level 10000 ❌ (5000 ≤ 10000)
    }
    
    void update_hierarchy_value() {
        previous_hierarchy_value = this_thread_hierarchy_value;  // Save current level
        this_thread_hierarchy_value = hierarchy_value;          // Drop to mutex's level
        // Example: Thread at 10000 locks mutex at 5000 → thread now operates at level 5000
    }
    
public:
    explicit hierarchical_mutex(unsigned long value)
        : hierarchy_value(value), previous_hierarchy_value(0) {}
    
    void lock() {
        check_for_hierarchy_violation();  // Step 1: Can we lock this level?
        internal_mutex.lock();           // Step 2: Actually acquire mutex
        update_hierarchy_value();        // Step 3: Update thread's level
    }
    
    void unlock() {
        this_thread_hierarchy_value = previous_hierarchy_value;  // Step 1: Restore previous level
        internal_mutex.unlock();                                // Step 2: Release actual mutex
        // CRITICAL: Restore previous level, not ULONG_MAX!
    }
    
    bool try_lock() {
        check_for_hierarchy_violation();
        if (!internal_mutex.try_lock())
            return false;
        update_hierarchy_value();
        return true;
    }
};

// Each thread starts at maximum level - can lock any mutex initially
thread_local unsigned long 
hierarchical_mutex::this_thread_hierarchy_value(ULONG_MAX);

// HIERARCHICAL DESIGN EXAMPLE:
hierarchical_mutex database_lock(10000);      // Database level (highest)
hierarchical_mutex table_lock(5000);          // Table level  
hierarchical_mutex row_lock(1000);            // Row level
hierarchical_mutex index_lock(500);           // Index level (lowest)

void good_hierarchical_usage() {
    // Thread starts at ULONG_MAX level
    std::lock_guard<hierarchical_mutex> db_guard(database_lock);    // Now at level 10000
    std::lock_guard<hierarchical_mutex> table_guard(table_lock);    // Now at level 5000 ✅
    std::lock_guard<hierarchical_mutex> row_guard(row_lock);        // Now at level 1000 ✅
    // Pattern: 10000 → 5000 → 1000 (always descending)
}

void bad_hierarchical_usage() {
    std::lock_guard<hierarchical_mutex> row_guard(row_lock);        // Now at level 1000
    // std::lock_guard<hierarchical_mutex> db_lock(database_lock);    // THROWS! 1000 ≤ 10000
    // Trying to go UP the hierarchy = VIOLATION
}

// WHY DEADLOCK IS IMPOSSIBLE:
// Thread A at level 5000 can only want levels < 5000
// Thread B at level 1000 can only want levels < 1000  
// → Thread A cannot want what Thread B holds
// → Thread B cannot want what Thread A holds
// → No circular dependency possible!

// REAL-WORLD HIERARCHY EXAMPLES:
// GUI App:     Window Manager(10000) → Window(5000) → Widget(1000) → Render(500)
// Web Server:  Server(10000) → Connection Pool(5000) → Request(1000) → Cache(500)
// Game Engine: World(10000) → Scene(5000) → Entity(1000) → Component(500)

// ============================================================================
// EXTENDING BEYOND LOCKS
// Same principles apply to ANY synchronization that can cause waiting
// ============================================================================

void thread_hierarchy_example() {
    std::thread worker1, worker2, worker3;
    
    // GOOD: Join threads in same function that started them
    // Establishes clear hierarchy - main waits for workers
    worker1.join();
    worker2.join(); 
    worker3.join();
}

void bad_thread_waiting() {
    std::mutex mtx;
    std::thread worker;
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        // BAD: Waiting for thread while holding lock
        // worker.join();  // Worker might need this lock to finish!
    }
    
    // GOOD: Release lock before waiting
    worker.join();
}

// ============================================================================
// KEY TAKEAWAYS:
// 1. Deadlock occurs when threads wait for each other in a cycle
// 2. Avoid nested locks - use std::lock for multiple locks
// 3. Don't call user code while holding locks
// 4. Always acquire multiple locks in consistent order
// 5. Consider lock hierarchy for complex applications
// 6. Apply same principles to threads and other synchronization
// 7. Design to avoid deadlock, don't just detect it
// ============================================================================


int main() {

    return 0;
}