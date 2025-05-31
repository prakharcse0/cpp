#include <mutex>
#include <thread>
#include <iostream>
#include <vector>

// =============================================================================
// COMPREHENSIVE std::unique_lock GUIDE
// =============================================================================

class some_big_object {
    int data = 42;
public:
    void swap_data(some_big_object& other) {
        std::swap(data, other.data);
    }
};

// =============================================================================
// 1. BASIC UNIQUE_LOCK VS LOCK_GUARD COMPARISON
// =============================================================================

class BasicExample {
private:
    std::mutex m;
    int shared_data = 0;

public:
    // Using lock_guard (simpler, less flexible)
    void increment_with_lock_guard() {
        std::lock_guard<std::mutex> lock(m); // Locks immediately, unlocks in destructor
        ++shared_data;
        // Lock automatically released when 'lock' goes out of scope
    }

    // Using unique_lock (more flexible, slightly more overhead)
    void increment_with_unique_lock() {
        std::unique_lock<std::mutex> lock(m); // Locks immediately by default
        ++shared_data;
        // Lock automatically released when 'lock' goes out of scope
        // BUT: unique_lock has additional capabilities (manual unlock, defer, transfer)
    }
};

// =============================================================================
// 2. DEFER_LOCK: ACQUIRE WITHOUT LOCKING
// =============================================================================

class DeferLockExample {
private:
    std::mutex m1, m2;
    int data1 = 0, data2 = 0;

public:
    void demonstrate_defer_lock() {
        // CRITICAL: std::defer_lock means "associate with mutex but DON'T lock yet"
        std::unique_lock<std::mutex> lock1(m1, std::defer_lock); // mutex m1 is NOT locked
        std::unique_lock<std::mutex> lock2(m2, std::defer_lock); // mutex m2 is NOT locked
        
        // At this point: 
        // - lock1 "knows about" m1 but hasn't locked it
        // - lock2 "knows about" m2 but hasn't locked it  
        // - OTHER THREADS CAN STILL LOCK m1 and m2!
        
        // Now lock both simultaneously (avoids deadlock)
        std::lock(lock1, lock2); // Locks both mutexes atomically
        
        // Now both mutexes are locked and owned by respective unique_locks
        data1 = 10;
        data2 = 20;
        
        // Locks released when unique_locks go out of scope
    }
};

// =============================================================================
// 3. ALL LOCKING/UNLOCKING METHODS
// =============================================================================

class LockingMethods {
private:
    std::mutex m;
    int data = 0;

public:
    void demonstrate_all_methods() {
        std::unique_lock<std::mutex> lk(m, std::defer_lock);
        
        // METHOD 1: Manual locking via unique_lock
        lk.lock();                    // Locks the mutex
        data++;
        lk.unlock();                  // Manually unlock (optional)
        
        // METHOD 2: Try lock (non-blocking)
        if (lk.try_lock()) {          // Attempts to lock, returns true if successful
            data++;
            lk.unlock();
        }
        
        // METHOD 3: Using std::lock with multiple unique_lock objects
        std::unique_lock<std::mutex> lk2(m, std::defer_lock);
        std::unique_lock<std::mutex> lk3(m, std::defer_lock);
        
        // IMPORTANT SYNTAX RULES:
        // **Correct usage**: std::lock(lock1, lock2) or std::lock(lock1, lock2, lock3, ...)
        // **Wrong usage**: std::lock(single_lock) - COMPILER ERROR!
        // std::lock() requires AT LEAST 2 arguments - cannot use with single lock!
        // std::lock(lk2, lk3);        // This would work if we had 2 different mutexes
        
        // For single mutex, use these valid syntaxes:
        lk2.lock();                   // lk.lock() is VALID syntax - direct method call
        data++;
        lk2.unlock();
        
        // NOTE: There's NO std::try_lock(lk1, lk2) function!
        // For try_lock with multiple locks, you need to do it manually:
        if (lk2.try_lock() && lk3.try_lock()) {
            // Both acquired successfully
            data++;
            lk3.unlock();
            lk2.unlock();
        }
        
        // METHOD 4: Automatic locking in constructor
        std::unique_lock<std::mutex> lk4(m); // Locks immediately
        data++;
        // Automatic unlock in destructor
    }
    
    // NOTE: You CANNOT do std::unlock(lk) - there's no such function!
    // Only lk.unlock() or let destructor handle it
};

// =============================================================================
// 4. OWNERSHIP TRANSFER (MOVE SEMANTICS)
// =============================================================================

class OwnershipTransfer {
private:
    std::mutex m;
    std::vector<int> protected_data;

public:
    // Function that returns a locked unique_lock
    std::unique_lock<std::mutex> get_lock() {
        std::unique_lock<std::mutex> lk(m);    // Lock acquired
        protected_data.push_back(1);           // Prepare some data
        return lk;                             // Ownership transferred (move semantics)
        // lk destructor does NOT unlock because ownership was moved
    }
    
    void process_with_transferred_lock() {
        std::unique_lock<std::mutex> my_lock = get_lock(); // Receive ownership
        // The mutex is still locked! Ownership transferred to my_lock
        protected_data.push_back(2);
        // my_lock destructor will unlock when it goes out of scope
    }
    
    // Explicit move example
    void explicit_move_example() {
        std::unique_lock<std::mutex> lk1(m);
        // std::unique_lock<std::mutex> lk2 = lk1;           // ERROR: Not copyable
        std::unique_lock<std::mutex> lk2 = std::move(lk1);   // OK: Movable
        
        // Now lk1 is empty (doesn't own any mutex)
        // lk2 owns the mutex and will unlock in its destructor
    }
};

// =============================================================================
// 5. ADVANCED FEATURES AND QUERIES
// =============================================================================

class AdvancedFeatures {
private:
    std::mutex m;
    int data = 0;

public:
    void demonstrate_ownership_queries() {
        std::unique_lock<std::mutex> lk(m, std::defer_lock);
        
        std::cout << "owns_lock() before: " << lk.owns_lock() << std::endl;  // false
        
        lk.lock();
        std::cout << "owns_lock() after lock: " << lk.owns_lock() << std::endl; // true
        
        lk.unlock();
        std::cout << "owns_lock() after unlock: " << lk.owns_lock() << std::endl; // false
        
        // You can check ownership and conditionally unlock
        if (lk.owns_lock()) {
            lk.unlock();
        }
    }
    
    void conditional_early_release() {
        std::unique_lock<std::mutex> lk(m);
        
        // Do some work that requires the lock
        data = 42;
        
        // If we're done early and don't need the lock anymore
        if (data == 42) {
            lk.unlock();  // Release early for better performance
            // Other threads can now acquire the mutex
            // Do other work that doesn't need the lock
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // lk destructor won't try to unlock since we already did
    }
};

// =============================================================================
// 6. SWAP OPERATION EXAMPLE (from the document)
// =============================================================================

class X {
private:
    some_big_object some_detail;
    std::mutex m;

public:
    X(some_big_object const& sd) : some_detail(sd) {}
    
    friend void swap(X& lhs, X& rhs) {
        if (&lhs == &rhs) return;
        
        // Create unique_locks but don't lock yet
        std::unique_lock<std::mutex> lock_a(lhs.m, std::defer_lock);
        std::unique_lock<std::mutex> lock_b(rhs.m, std::defer_lock);
        
        // Lock both simultaneously (prevents deadlock)
        std::lock(lock_a, lock_b);
        
        // Now both objects are safely locked, perform swap
        lhs.some_detail.swap_data(rhs.some_detail);
        
        // Locks automatically released when unique_locks go out of scope
    }
};

// =============================================================================
// 7. FUNCTION TAKING UNIQUE_LOCK AS PARAMETER
// =============================================================================

class ParameterExample {
private:
    std::mutex m;
    std::vector<int> shared_data;

public:
    // Function that accepts a unique_lock as parameter
    // This enforces that caller must have the lock before calling
    void process_with_lock(std::unique_lock<std::mutex>& lock) {
        // Verify the caller actually has the lock
        if (!lock.owns_lock()) {
            throw std::runtime_error("Lock must be owned before calling this function!");
        }
        
        // Safe to access shared data - lock is guaranteed
        shared_data.push_back(42);
        shared_data.push_back(100);
        
        // Function can also release lock early if needed
        lock.unlock();
        
        // Do some work that doesn't need the lock
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    void caller_example() {
        std::unique_lock<std::mutex> my_lock(m);  // Acquire lock
        process_with_lock(my_lock);               // Pass lock to function
        
        // Note: my_lock might be unlocked now (depends on what function did)
        if (!my_lock.owns_lock()) {
            std::cout << "Function released the lock early" << std::endl;
        }
    }
    
    // Another example: Function that requires lock but might need to unlock/relock
    void complex_operation(std::unique_lock<std::mutex>& lock) {
        // Phase 1: Work with shared data
        shared_data.push_back(1);
        
        // Phase 2: Release lock for expensive operation
        lock.unlock();
        expensive_computation();  // Don't hold lock during this
        
        // Phase 3: Reacquire lock for final update
        lock.lock();
        shared_data.push_back(2);
    }
    
private:
    void expensive_computation() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
};

// =============================================================================
// 8. PERFORMANCE AND SIZE CONSIDERATIONS + WHY UNIQUE_LOCK IS BETTER
// =============================================================================

/*
IMPORTANT NOTES:

1. SIZE AND PERFORMANCE:
   - std::unique_lock is LARGER than std::lock_guard
   - std::unique_lock is SLOWER than std::lock_guard  
   - Reason: unique_lock stores a flag to track ownership state
   - Use lock_guard when you don't need the extra flexibility

2. LOCK DESTRUCTION AND SCOPE:
   - YES, locks are automatically destroyed when going out of scope
   - Destructor calls unlock() ONLY if owns_lock() returns true
   - This is why ownership tracking is necessary

3. DEFER_LOCK BEHAVIOR:
   - When you create unique_lock with defer_lock, the mutex is NOT locked
   - OTHER THREADS CAN STILL LOCK THAT MUTEX
   - It's like saying "I want to work with this mutex later, but don't lock it now"

4. BENEFITS OF DEFER_LOCK:
   - Allows you to create multiple unique_locks and then lock them all at once
   - Prevents deadlock when you need multiple mutexes
   - Example: std::lock(lock1, lock2, lock3) locks all atomically
   
5. WHY NOT LOCK IMMEDIATELY?
   - If you have multiple mutexes to lock, locking them one by one can cause deadlock
   - Thread A locks mutex1 then tries mutex2
   - Thread B locks mutex2 then tries mutex1
   - = DEADLOCK!
   - Solution: defer_lock + std::lock() = atomic locking of multiple mutexes

6. SCENARIOS WHERE UNIQUE_LOCK IS MUCH BETTER THAN NORMAL MUTEX/LOCK_GUARD:
   
   a) EARLY RELEASE FOR PERFORMANCE:
      - Can unlock before destructor to reduce contention
      - Critical for high-performance applications
      - mutex.lock() + manual unlock = error-prone (what if exception?)
      - unique_lock = safe early release + automatic cleanup
   
   b) CONDITIONAL LOCKING:
      - try_lock() without blocking
      - mutex.try_lock() requires manual unlock tracking
      - unique_lock.try_lock() = automatic cleanup even on failure paths
   
   c) LOCK OWNERSHIP TRANSFER:
      - Pass locked state between functions
      - Impossible with raw mutex (can't transfer ownership)
      - lock_guard is not movable
      - Only unique_lock supports this pattern
   
   d) DEADLOCK-FREE MULTI-MUTEX LOCKING:
      - std::lock() works with unique_lock objects
      - Raw mutexes work too, but no RAII cleanup
      - lock_guard can't defer locking
      - unique_lock = deadlock prevention + RAII
   
   e) GENERIC PROGRAMMING:
      - Template functions that need lock/unlock/try_lock interface
      - unique_lock provides all mutex methods
      - Can be passed to generic algorithms expecting lockable objects
   
   f) EXCEPTION SAFETY:
      - Complex locking logic with exceptions
      - Manual mutex management = potential leaks
      - unique_lock guarantees cleanup even with exceptions
   
   g) TIMED LOCKING:
      - try_lock_for(), try_lock_until() methods
      - Not available with basic lock_guard
      - Better than raw mutex (no manual cleanup needed)

7. WHEN TO USE EACH:
   - Raw mutex: Simple cases, performance-critical, manual control
   - lock_guard: Simple RAII, no special requirements
   - unique_lock: Any of the above scenarios, complex locking patterns
*/

int main() {
    // Demonstrate the examples
    BasicExample basic;
    basic.increment_with_lock_guard();
    basic.increment_with_unique_lock();
    
    DeferLockExample defer;
    defer.demonstrate_defer_lock();
    
    LockingMethods methods;
    methods.demonstrate_all_methods();
    
    OwnershipTransfer transfer;
    transfer.process_with_transferred_lock();
    
    AdvancedFeatures advanced;
    advanced.demonstrate_ownership_queries();
    advanced.conditional_early_release();
    
    return 0;
}