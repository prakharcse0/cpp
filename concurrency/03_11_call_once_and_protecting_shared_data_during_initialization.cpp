// call_once, once_flag, 

/*
=============================================================================
3.3 ALTERNATIVE FACILITIES FOR PROTECTING SHARED DATA
=============================================================================

KEY CONCEPT: Mutexes aren't always the most appropriate protection mechanism.
Some scenarios require specialized approaches for better performance and correctness.

MAIN SCENARIO: Data that needs protection ONLY during initialization
- After initialization: no explicit synchronization needed
- Reasons: data becomes read-only OR operations provide implicit protection
- Problem: Using mutex after initialization = unnecessary performance hit

=============================================================================
3.3.1 PROTECTING SHARED DATA DURING INITIALIZATION
=============================================================================
*/

#include <memory>
#include <mutex>
#include <thread>
#include <iostream>
#include <vector>
#include <chrono>

// ============= SIMPLE RESOURCE CLASS FOR DEMONSTRATION =============
class some_resource {
private:
    int data;
public:
    some_resource() : data(42) {
        // Simulate expensive construction
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    void do_something() {
        std::cout << "Resource working with data: " << data << std::endl;
    }
    
    int get_data() const { return data; }
};

// Simple connection classes for later examples
struct connection_info {
    std::string host = "localhost";
    int port = 8080;
};

struct connection_handle {
    bool connected = false;
    void send_data(const std::string& data) { 
        std::cout << "Sending: " << data << std::endl; 
    }
    std::string receive_data() { 
        return "received_data"; 
    }
};

struct data_packet {
    std::string payload = "sample_data";
};

// Mock connection manager
class connection_manager_t {
public:
    connection_handle open(const connection_info& info) {
        connection_handle handle;
        handle.connected = true;
        std::cout << "Connection opened to " << info.host << ":" << info.port << std::endl;
        return handle;
    }
} connection_manager;

// ============= PROBLEM: NAIVE APPROACH =============
// Single-threaded lazy initialization (NOT thread-safe)
std::shared_ptr<some_resource> resource_ptr;

void foo_unsafe() {
    if (!resource_ptr) {                    // Race condition possible
        resource_ptr.reset(new some_resource);  // Multiple threads may initialize
    }
    resource_ptr->do_something();
}


// ============= SOLUTION 1: MUTEX APPROACH (INEFFICIENT) =============
std::shared_ptr<some_resource> resource_ptr_mutex;
std::mutex resource_mutex;

void foo_with_mutex() {
    std::unique_lock<std::mutex> lk(resource_mutex);  // ALL threads serialized here
    if (!resource_ptr_mutex) {                        // Only initialization needs protection
        resource_ptr_mutex.reset(new some_resource);
    }
    lk.unlock();
    resource_ptr_mutex->do_something();
}

/*
PROBLEM WITH MUTEX APPROACH:
- Every thread must wait on mutex just to CHECK if initialization is done
- Unnecessary serialization after initialization is complete
- Performance hit scales with number of threads
*/

// ============= ANTI-PATTERN: DOUBLE-CHECKED LOCKING (UNDEFINED BEHAVIOR) =============
void undefined_behaviour_with_double_checked_locking() {
    if (!resource_ptr) {                           // First check (unsynchronized read)
        std::lock_guard<std::mutex> lk(resource_mutex);
        if (!resource_ptr) {                       // Second check (double-checked)
            resource_ptr.reset(new some_resource); // Write inside lock
        }
    }
    resource_ptr->do_something();  // UNDEFINED BEHAVIOR possible here
}

/*
=============================================================================
DETAILED ANALYSIS: WHY DOUBLE-CHECKED LOCKING IS CATASTROPHICALLY BROKEN
=============================================================================

PROBLEM 1: MEMORY REORDERING
What programmer expects:
1. Allocate memory for some_resource
2. Call some_resource constructor  
3. Assign pointer to resource_ptr

What compiler/CPU can actually do:
1. Allocate memory for some_resource
2. Assign pointer to resource_ptr  ← REORDERED! Visible to other threads!
3. Call some_resource constructor   ← Object not yet constructed!

PROBLEM 2: THE RACE CONDITION TIMELINE
Time | Thread A (initializing)     | Thread B (using)
-----|-----------------------------|--------------------------
T1   | if (!resource_ptr)          |
T2   | → null, enters lock         |
T3   | allocates memory            |
T4   | assigns resource_ptr        | if (!resource_ptr)
T5   | [constructor running...]    | → sees NON-NULL pointer!
T6   |                             | → skips lock entirely
T7   |                             | resource_ptr->do_something()
T8   |                             | ❌ ACCESSES UNINITIALIZED MEMORY!

PROBLEM 3: DATA RACE = UNDEFINED BEHAVIOR
- Thread A WRITES to resource_ptr inside the lock (synchronized)
- Thread B READS resource_ptr outside the lock (unsynchronized)  
- Concurrent unsynchronized access = DATA RACE
- C++ Standard: DATA RACE = UNDEFINED BEHAVIOR
- Compiler assumes no data races exist → aggressive optimizations break logic

PROBLEM 4: WHY "SMART" FIXES DON'T WORK
- shared_ptr doesn't help: Assignment itself isn't atomic
- volatile doesn't help: Doesn't provide synchronization
- More checks don't help: Fundamental race condition remains

THE CORE ISSUE: Double-checked locking tries to optimize the rare case 
(initialization) at the cost of correctness. It violates the fundamental 
rule that all access to shared data must be properly synchronized.
*/

// ============= SOLUTION 2: std::call_once (RECOMMENDED) =============
#include <mutex>  // for std::once_flag and std::call_once

std::shared_ptr<some_resource> resource_ptr_once;
std::once_flag resource_flag;  // Flag to ensure single initialization

void init_resource() {
    resource_ptr_once.reset(new some_resource);  // Called exactly once
}

void foo_with_call_once() {
    std::call_once(resource_flag, init_resource);  // Thread-safe, efficient
    resource_ptr_once->do_something();             // Safe to use after call_once returns
}

/*
=============================================================================
DETAILED EXPLANATION: std::call_once AND std::once_flag
=============================================================================

std::once_flag:
- A synchronization primitive that ensures something happens exactly once
- Maintains internal state to track whether the operation has completed
- NOT copyable, NOT movable (like std::mutex)
- Thread-safe: multiple threads can use the same once_flag simultaneously
- Lightweight: typically just a few bytes of memory

std::call_once(flag, callable, args...):
- Executes callable exactly once across all threads using the same flag
- If multiple threads call simultaneously, only ONE thread executes callable
- Other threads BLOCK until the execution completes
- After first execution, subsequent calls return immediately (fast path)
- Provides proper memory synchronization (happens-before relationship)
- Works with: functions, lambdas, function objects, member functions

INTERNAL BEHAVIOR:
1. First thread to call std::call_once wins and executes the callable
2. Other threads block and wait for completion
3. If callable throws exception, flag remains "not called" 
4. Another thread can then attempt the initialization
5. Once callable completes successfully, flag is marked "called"
6. All future calls return immediately without blocking

MEMORY SYNCHRONIZATION GUARANTEES:
- Everything done by the callable "happens-before" subsequent calls return
- No data races between initialization and usage
- Proper compiler/CPU memory barrier insertion
*/

// ============= SOLUTION 2B: std::call_once WITH CLASS MEMBERS =============
class ResourceManager {
private:
    std::shared_ptr<some_resource> resource;
    mutable std::once_flag init_flag;  // mutable because called from const methods
    
    // Private initialization method
    void initialize_resource() const {  // const because called from const methods
        // Note: we modify resource through const method, but that's okay because
        // we're modifying the CONTENT of shared_ptr, not the shared_ptr itself
        const_cast<ResourceManager*>(this)->resource.reset(new some_resource());
        std::cout << "Resource initialized by thread: " 
                  << std::this_thread::get_id() << std::endl;
    }

public:
    ResourceManager() = default;
    
    // Can be called from multiple threads safely
    void use_resource() const {
        std::call_once(init_flag, &ResourceManager::initialize_resource, this);
        resource->do_something();
    }
    
    // Alternative: using lambda for initialization
    void use_resource_lambda() const {
        std::call_once(init_flag, [this]() {
            const_cast<ResourceManager*>(this)->resource.reset(new some_resource());
            std::cout << "Resource initialized via lambda by thread: " 
                      << std::this_thread::get_id() << std::endl;
        });
        resource->do_something();
    }
    
    // Another method that might trigger initialization
    int get_resource_data() const {
        std::call_once(init_flag, &ResourceManager::initialize_resource, this);
        return resource->get_data();
    }
};

/*
=============================================================================
COMPREHENSIVE EXPLANATION: THE 'mutable' KEYWORD
=============================================================================

WHAT IS 'mutable'?
A keyword that allows modification of specific class members even within const methods.

BASIC RULE:
- const methods cannot modify any member variables
- EXCEPTION: members marked as 'mutable' CAN be modified in const methods

WHY IS IT NEEDED?
Some operations are logically const (don't change observable state) but need to
modify internal implementation details like caches, locks, or lazy initialization.

THREAD SAFETY WITH mutable:
- mutable doesn't make operations thread-safe automatically
- Still need proper synchronization (mutex, atomic, etc.)
- Common pattern: mutable std::mutex for const method thread safety

WHEN NOT TO USE mutable:
- Don't use it to bypass const-correctness for actual state changes
- Avoid if it makes the class harder to reason about
- Not for fundamental object properties that should trigger non-const methods

MEMORY IMPACT:
- No runtime overhead
- Compile-time feature only
- Doesn't change object layout or size
*/

/*
CLASS-BASED std::call_once NOTES:
- once_flag should be mutable if used in const methods
- Member function requires 'this' pointer as argument to std::call_once
- Can use lambdas to capture 'this' instead of member function pointers
- Any method can trigger initialization, but it happens exactly once
- Initialization is lazy: only happens when first needed
- Thread-safe: multiple threads can call any method simultaneously
*/

// ============= ADVANCED: std::call_once WITH PARAMETERS =============
class ConfigurableResource {
private:
    std::shared_ptr<some_resource> resource;
    mutable std::once_flag init_flag;
    std::string config_name;
    
public:
    ConfigurableResource(const std::string& name) : config_name(name) {}
    
    void initialize_with_config(const std::string& config) const {
        // Pass additional parameters to the initialization function
        std::call_once(init_flag, [this, config]() {
            const_cast<ConfigurableResource*>(this)->resource.reset(new some_resource());
            std::cout << "Initialized " << config_name 
                      << " with config: " << config << std::endl;
        });
    }
    
    void use_resource(const std::string& config = "default") const {
        initialize_with_config(config);
        resource->do_something();
    }
};

/*
PARAMETER PASSING WITH std::call_once:
- Additional arguments to std::call_once are forwarded to the callable
- For member functions: std::call_once(flag, &Class::method, this, arg1, arg2, ...)
- For lambdas: capture what you need, pass rest as parameters
- Parameters are perfectly forwarded (supports move semantics)
*/

/*
ADVANTAGES OF std::call_once:
- Lower overhead than explicit mutex, especially after initialization
- Guaranteed exactly-once execution across all threads  
- Proper synchronization without race conditions
- Works with any callable: functions, lambdas, function objects, member functions
- Exception-safe: if initialization throws, another thread can retry
- Fast path: after initialization, calls have minimal overhead
- Memory efficient: once_flag is lightweight compared to mutex + bool
*/


// ============= SOLUTION 3: CLASS MEMBER INITIALIZATION =============
// Class Member Lazy Initialization with std::call_once
class DatabaseConnection {
private:
    connection_info connection_details;
    connection_handle connection;           // Expensive resource
    std::once_flag connection_init_flag;    // Ensures single initialization

    void open_connection() {
        connection = connection_manager.open(connection_details);
    }

public:
    DatabaseConnection(connection_info const& details)
        : connection_details(details) {}    // Constructor is cheap - no connection yet

    void send_data(data_packet const& data) {
        // Lazy init: create connection only when first needed, thread-safe
        std::call_once(connection_init_flag, &DatabaseConnection::open_connection, this);
        connection.send_data(data.payload);
    }

    data_packet receive_data() {
        // Same initialization from any method - happens exactly once
        std::call_once(connection_init_flag, &DatabaseConnection::open_connection, this);
        data_packet packet;
        packet.payload = connection.receive_data();  // Convert string to data_packet
        return packet;
    }
};

/*
KEY POINTS:
- Constructor stores config only, doesn't create expensive resource
- First method call (send_data OR receive_data) triggers initialization  
- Multiple threads safe: only one initializes, others wait
- After init: std::call_once has minimal overhead
- Syntax: std::call_once(flag, &Class::method, this)
*/

// ============= SOLUTION 4: STATIC LOCAL VARIABLE (C++11 THREAD-SAFE) =============
/*
LAZY SINGLETON PATTERN: Object created only when first accessed, not at program startup

KEY C++11 GUARANTEE: Static local initialization is thread-safe
- Only ONE thread executes the constructor
- Other threads BLOCK until initialization completes  
- No race condition on initialization process
- Only race is over WHICH thread initializes (harmless)

TIMELINE:
1. Program starts: instance doesn't exist yet
2. First function call: static variable gets constructed
3. Subsequent calls: returns reference to existing instance

ADVANTAGES:
- Lazy initialization: created on-demand, not at startup
- Thread-safe without explicit synchronization
- Compiler-optimized, minimal overhead
- Avoids static initialization order problems
- Perfect for singleton pattern
*/

class ResourceManager_2 {
public:
    ResourceManager_2() {
        std::cout << "ResourceManager_2 constructed by thread: " 
                  << std::this_thread::get_id() << std::endl;
    }
    
    void do_work() { 
        std::cout << "ResourceManager_2 doing work" << std::endl; 
    }
    
    // OPTION 1: Static method (most common for singletons)
    static ResourceManager_2& getInstance() {
        static ResourceManager_2 instance;  // LAZY: Created only on first call
                                        // THREAD-SAFE: C++11+ guarantees single construction
        return instance;                 // Always returns same instance
    }

    // Prevent copying for proper singleton
    ResourceManager_2(const ResourceManager_2&) = delete;
    ResourceManager_2& operator=(const ResourceManager_2&) = delete;
}; 

// Option 2
ResourceManager_2& get_resource_manager() {
    static ResourceManager_2 instance;  // LAZY: Created only on first call
                                     // THREAD-SAFE: C++11+ guarantees single construction
    return instance;                 // Always returns same instance
}

/*
USAGE: Multiple threads can safely call - all get same instance
void worker_thread() {
    ResourceManager& rm = get_resource_manager();  // Thread-safe access
    rm.do_work();
}

COMPARED TO std::call_once:
- Static local: Compiler-optimized, singleton pattern, simpler syntax
- std::call_once: More flexible, works with class members, any initialization

PRE-C++11 PROBLEM: Race condition - multiple threads could initialize simultaneously
C++11+ SOLUTION: Standard guarantees exactly-once initialization across all threads
*/

/*
STATIC LOCAL VARIABLE GUARANTEES (C++11+):
- Initialization happens on exactly ONE thread
- All other threads wait until initialization completes
- No race condition on the initialization process
- Race only over WHICH thread does initialization (harmless)
- Alternative to std::call_once for singleton pattern
*/


// ============= PERFORMANCE COMPARISON =============
void performance_analysis() {
    /*
    PERFORMANCE RANKING (best to worst):
    1. Static local variable: Compiler-optimized, minimal overhead
    2. std::call_once: Optimized for post-initialization calls
    3. Mutex approach: High overhead, serializes all threads
    4. Double-checked locking: UNDEFINED BEHAVIOR - never use
    
    WHEN TO USE EACH:
    - Static local: Singleton pattern, global instances
    - std::call_once: Class members, complex initialization logic
    - Mutex: When you need more control over locking scope
    */
}

// ============= COMPLETE EXAMPLE: THREAD-SAFE LAZY SINGLETON =============
class ExpensiveResource {
private:
    int value;
    ExpensiveResource() : value(123) { 
        std::cout << "ExpensiveResource constructed" << std::endl;
    }
    
public:
    static ExpensiveResource& getInstance() {
        static ExpensiveResource instance;  // Thread-safe in C++11+
        return instance;
    }
    
    void doWork() { 
        std::cout << "Working with value: " << value << std::endl; 
    }
    
    // Prevent copying
    ExpensiveResource(const ExpensiveResource&) = delete;
    ExpensiveResource& operator=(const ExpensiveResource&) = delete;
};

// Usage from multiple threads - completely safe
void worker_thread() {
    ExpensiveResource& resource = ExpensiveResource::getInstance();
    resource.doWork();
}

// ============= DEMONSTRATION FUNCTION =============
void demonstrate_all_approaches() {
    std::cout << "\n=== Demonstrating Thread-Safe Initialization ===" << std::endl;
    
    // Test static local variable approach
    std::cout << "\n1. Static local variable approach:" << std::endl;
    std::vector<std::thread> threads1;
    for (int i = 0; i < 3; ++i) {
        threads1.emplace_back(worker_thread);
    }
    for (auto& t : threads1) t.join();
    
    // Test std::call_once approach
    std::cout << "\n2. std::call_once approach:" << std::endl;
    std::vector<std::thread> threads2;
    for (int i = 0; i < 3; ++i) {
        threads2.emplace_back(foo_with_call_once);
    }
    for (auto& t : threads2) t.join();
    
    // Test class-based std::call_once
    std::cout << "\n3. Class-based std::call_once:" << std::endl;
    ResourceManager rm;
    std::vector<std::thread> threads3;
    for (int i = 0; i < 3; ++i) {
        threads3.emplace_back([&rm]() {
            rm.use_resource();
        });
    }
    for (auto& t : threads3) t.join();
    
    // Test configurable resource
    std::cout << "\n4. Configurable resource with std::call_once:" << std::endl;
    ConfigurableResource cr("Database");
    std::vector<std::thread> threads4;
    for (int i = 0; i < 3; ++i) {
        threads4.emplace_back([&cr, i]() {
            cr.use_resource("config_" + std::to_string(i));
        });
    }
    for (auto& t : threads4) t.join();
    
    // Test class member initialization (original example)
    std::cout << "\n5. DatabaseConnection class member initialization:" << std::endl;
    DatabaseConnection db_conn({});
    std::vector<std::thread> threads5;
    for (int i = 0; i < 3; ++i) {
        threads5.emplace_back([&db_conn]() {
            data_packet packet;
            db_conn.send_data(packet);
        });
    }
    for (auto& t : threads5) t.join();
    
    std::cout << "\nAll demonstrations completed successfully!" << std::endl;
}

/*
=============================================================================
KEY TAKEAWAYS:
=============================================================================

1. IDENTIFY THE PATTERN: Data needing protection only during initialization
2. AVOID MUTEX FOR POST-INITIALIZATION: Unnecessary performance penalty
3. NEVER USE DOUBLE-CHECKED LOCKING: Undefined behavior due to data races
4. PREFER std::call_once: Designed specifically for this use case
5. CONSIDER STATIC LOCALS: Thread-safe in C++11+ for singletons
6. REMEMBER MOVE/COPY RESTRICTIONS: std::once_flag is not copyable/movable

BROADER PRINCIPLE: This is a special case of "rarely updated data structures"
- Most access is read-only (concurrent access safe)
- Occasional updates need synchronization
- Need mechanisms that acknowledge this read-heavy pattern

TESTING: Uncomment the line below to run demonstrations
// int main() { demonstrate_all_approaches(); return 0; }
*/

int main() {

    return 0;
}