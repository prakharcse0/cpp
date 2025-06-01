#include <mutex>
#include <iostream>

/*
KEY TAKEAWAYS:

1. std::mutex: Same thread locking twice = undefined behavior (deadlock/crash)
2. std::recursive_mutex: Same thread CAN lock multiple times safely
3. Must unlock() same number of times as lock() calls
4. lock_guard and unique_lock handle unlock counting automatically
5. Usually indicates design problem - prefer refactoring over recursive mutex
6. Class invariants often broken during nested calls with recursive mutex
7. Extract private unlocked functions instead of using recursive mutex

WHEN RECURSIVE MUTEX SEEMS NEEDED:
- Public method A locks mutex, calls public method B
- Method B also tries to lock same mutex
- With std::mutex: undefined behavior
- With std::recursive_mutex: works but not recommended

BETTER DESIGN:
- Private functions assume mutex already locked
- Public functions lock once, call private functions
- Clearer responsibility, safer invariants
*/

// Problem: std::mutex cannot be locked twice by same thread
class ProblemExample {
    std::mutex mtx;
    int value = 0;
    
public:
    void methodA() {
        std::lock_guard<std::mutex> lock(mtx);
        value += 10;
        // methodB();  // This would cause undefined behavior!
    }
    
    void methodB() {
        std::lock_guard<std::mutex> lock(mtx);  // Second lock attempt
        value *= 2;
    }
};

// Quick fix: recursive_mutex allows multiple locks from same thread
class RecursiveSolution {
    std::recursive_mutex mtx;  // Key difference: recursive_mutex
    int value = 0;
    
public:
    void methodA() {
        std::lock_guard<std::recursive_mutex> lock(mtx);  // First lock
        value += 10;
        methodB();  // This now works - second lock from same thread
    }
    
    void methodB() {
        std::lock_guard<std::recursive_mutex> lock(mtx);  // Second lock OK
        value *= 2;
    }  // Both locks automatically released when guards destruct
};

// Better design: private functions assume lock already held
class BetterDesign {
    std::mutex mtx;  // Back to regular mutex
    int value = 0;
    
    // Private: assumes caller already holds lock
    void methodB_unlocked() {
        value *= 2;  // No locking here
    }
    
public:
    void methodA() {
        std::lock_guard<std::mutex> lock(mtx);  // Single lock
        value += 10;
        methodB_unlocked();  // Call unlocked version
    }
    
    void methodB() {
        std::lock_guard<std::mutex> lock(mtx);  // Separate lock
        methodB_unlocked();  // Same unlocked logic
    }
};

// Why recursive mutex can be dangerous - Bank Account Example
class DangerousBankAccount {
    std::recursive_mutex mtx;
    int balance = 1000;
    
public:
    void transfer(int amount) {
        std::lock_guard<std::recursive_mutex> lock(mtx);
        
        balance -= amount;        // Temporarily negative! (e.g., 1000 - 1200 = -200)
        validateTransfer();       // Calls another method while balance is invalid
        balance += 5;             // Add transaction fee
    }
    
    void validateTransfer() {
        std::lock_guard<std::recursive_mutex> lock(mtx);
        // This method assumes it's called on a valid account
        // But transfer() called us while balance was negative!
        if (balance < 0) {
            // What should we do here? Is this a real problem or temporary?
            // We can't tell because we don't know we're in middle of transfer
            std::cout << "ERROR: Negative balance detected: " << balance << std::endl;
            // Should we throw exception? Log error? Fix it? Design is unclear!
        } else {
            std::cout << "Balance OK: " << balance << std::endl;
        }
    }
};

/*
WHY THIS IS DANGEROUS:

1. BROKEN CLASS INVARIANTS:
   - balance should never be negative for a valid account
   - transfer() temporarily breaks this (balance = -200)
   - This is normal during complex operations

2. THE HIDDEN PROBLEM:
   - transfer() calls validateTransfer() while balance is invalid
   - validateTransfer() assumes it's working with a valid account
   - Recursive locking allows this invalid call to happen

3. DESIGN CONFUSION:
   - validateTransfer() sees negative balance but doesn't know why
   - Is this a real error or just temporary state during transfer?
   - Should it fix the problem? Ignore it? Crash? Responsibility unclear!
   - With regular mutex, this would deadlock, forcing better design

4. REAL-WORLD CONSEQUENCES:
   - Method might log false errors
   - Might throw exceptions during valid operations  
   - Might corrupt data trying to "fix" temporary states
   - Code becomes unpredictable and hard to debug

5. WHY REGULAR MUTEX FORCES BETTER DESIGN:
    - Would deadlock, forcing you to create validateTransfer_unlocked()
    - Private method knows it's called during operations
    - Clear separation between "external validation" vs "internal validation during operations"


BETTER DESIGN WITH REGULAR MUTEX:

class SafeBankAccount {
    std::mutex mtx;
    int balance = 1000;
    
    // Private: assumes caller holds lock, knows about intermediate states
    void validateTransfer_unlocked() {
        // This method knows it's called during operations
        // It can handle intermediate states appropriately
        // Or simply skip validation during transfers
    }
    
public:
    void transfer(int amount) {
        std::lock_guard<std::mutex> lock(mtx);      // Single lock
        balance -= amount;
        validateTransfer_unlocked();                 // No additional locking
        balance += 5;
    }
    
    void validateTransfer() {
        std::lock_guard<std::mutex> lock(mtx);      // Separate operation
        validateTransfer_unlocked();
    }
};
*/

// Manual locking example
void manualExample() {
    std::recursive_mutex rmtx;
    
    rmtx.lock();    // Lock count: 1
    rmtx.lock();    // Lock count: 2  
    rmtx.lock();    // Lock count: 3
    
    // Must unlock same number of times
    rmtx.unlock();  // Lock count: 2
    rmtx.unlock();  // Lock count: 1
    rmtx.unlock();  // Lock count: 0 (fully unlocked)
}

int main() {
    RecursiveSolution rs;
    rs.methodA();  // Works with recursive mutex
    
    BetterDesign bd;
    bd.methodA();  // Better design without recursive mutex
    
    return 0;
}