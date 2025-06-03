#include <future>
#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <functional>

/*
=============================================================================
                    COMPLETE std::async AND std::future TUTORIAL
=============================================================================

WHAT ARE FUTURES?
- A future represents a value that will be available at some point in the future
- Like a restaurant ticket - you get it now, food comes later
- Two types: std::future<T> (unique) and std::shared_future<T> (shareable)
- Declared in <future> header

WHY USE FUTURES INSTEAD OF std::thread?
- std::thread can't easily return values from functions
- Futures provide clean way to get results from background tasks
- Better exception handling
- More flexible execution policies

FUTURE TYPES:
1. std::future<T>: One-time use, like std::unique_ptr
2. std::shared_future<T>: Multiple access, like std::shared_ptr
3. std::future<void>: For tasks that don't return values
*/

/*
=============================================================================
                                KEY TAKEAWAYS
=============================================================================

1. WHEN TO USE std::async:
   - Need result from background computation
   - Want simple async execution
   - Don't need fine control over threads

2. ARGUMENT PASSING RULES:
   - Regular functions: std::async(func, args...)
   - Member functions: std::async(&Class::method, object, args...)
   - References: Use std::ref() to avoid copying
   - Move semantics: Automatic for rvalue arguments

3. LAUNCH POLICIES:
   - std::launch::async: Force new thread
   - std::launch::deferred: Lazy evaluation
   - Default: Implementation chooses (usually deferred)

4. FUTURE OPERATIONS:
   - get(): Block and get result (one-time for std::future)
   - wait(): Block without getting result
   - wait_for(): Wait with timeout
   - valid(): Check if future is usable

5. EXCEPTION HANDLING:
   - Exceptions automatically transported across threads
   - get() re-throws exceptions from async tasks
   - Clean error handling for async operations

6. BEST PRACTICES:
   - Use std::launch::async when you need guaranteed async execution
   - Use std::shared_future when multiple threads need the same result
   - Always handle exceptions from async tasks
   - Check future validity when needed
=============================================================================
*/


//=============================================================================
// SECTION 1: BASIC std::async USAGE
//=============================================================================

// Simple function that simulates work and returns a value
int find_the_answer_to_ltuae() {
    std::cout << "[ASYNC TASK] Starting the ultimate calculation...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "[ASYNC TASK] Calculation complete!\n";
    return 42; // The answer to Life, the Universe, and Everything
}

void do_other_stuff() {
    std::cout << "[MAIN THREAD] Doing other important work...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "[MAIN THREAD] Other work complete!\n";
}

void demonstrate_basic_async() {
    std::cout << "\n=== BASIC std::async EXAMPLE ===\n";
    
    // START ASYNC TASK
    // std::async returns std::future<ReturnType>
    // By default, implementation chooses whether to run async or deferred
    std::future<int> the_answer = std::async(find_the_answer_to_ltuae);
    
    // We can do other work while calculation runs in background
    do_other_stuff();
    
    // When we need the result, call get() - this blocks until ready
    std::cout << "[MAIN THREAD] The answer is " << the_answer.get() << std::endl;
    
    // IMPORTANT: After get(), future becomes invalid!
    // the_answer.get(); // Would throw std::future_error
}

//=============================================================================
// SECTION 2: std::async ARGUMENT PASSING - DETAILED EXPLANATION
//=============================================================================

class X {
public:
    void foo(int value, std::string const& message) {
        std::cout << "[X::foo] Called with " << value << " and '" << message << "'\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::string bar(std::string const& input) {
        std::cout << "[X::bar] Processing: " << input << std::endl;
        return "Processed: " + input;
    }
};

struct Y {
    double operator()(double x) {
        std::cout << "[Y::operator()] Called with " << x << std::endl;
        return x * x;
    }
};

std::string baz(X& x_ref) {
    std::cout << "[baz] Called with X reference\n";
    return "baz processed X";
}

class move_only {
public:
    move_only() { std::cout << "[move_only] Constructed\n"; }
    move_only(move_only&& other) noexcept { std::cout << "[move_only] Moved\n"; }
    move_only(move_only const&) = delete;
    move_only& operator=(move_only&&) = default;
    move_only& operator=(move_only const&) = delete;
    
    void operator()() {
        std::cout << "[move_only] Executed\n";
    }
};

void demonstrate_async_arguments() {
    std::cout << "\n=== std::async ARGUMENT PASSING ===\n";
    
    /*
    ARGUMENT PASSING RULES:
    1. Regular functions: std::async(function, arg1, arg2, ...)
    2. Member functions: std::async(&Class::method, object, arg1, arg2, ...)
    3. For references: Use std::ref() to avoid copying
    4. Move semantics: rvalue arguments are moved automatically
    5. Lambda functions: std::async(lambda, args...)
    */
    
    // 1. MEMBER FUNCTION CALLS
    std::cout << "\n1. Member function calls:\n";
    X x;
    
    // Call X::foo through pointer to member function
    // Syntax: std::async(&Class::method, object_ptr_or_ref, args...)
    auto f1 = std::async(&X::foo, &x, 42, "hello");
    f1.get(); // Wait for void function to complete
    
    // Call X::bar and get return value
    // Pass object by copy (creates temporary copy)
    auto f2 = std::async(&X::bar, x, "goodbye");
    std::cout << "Result: " << f2.get() << "\n";
    
    // 2. CALLABLE OBJECTS (FUNCTORS)
    std::cout << "\n2. Callable objects:\n";
    Y y;
    
    // Create temporary Y object and call operator()
    auto f3 = std::async(Y(), 3.141);
    std::cout << "Y()(3.141) = " << f3.get() << std::endl;
    
    // Use existing object with std::ref to avoid copying
    auto f4 = std::async(std::ref(y), 2.718);
    std::cout << "y(2.718) = " << f4.get() << std::endl;
    
    // 3. FUNCTIONS WITH REFERENCES
    std::cout << "\n3. Functions with references:\n";
    // MUST use std::ref for reference parameters
    auto f5 = std::async(baz, std::ref(x));
    std::cout << "Result: " << f5.get() << std::endl;
    
    // 4. MOVE-ONLY TYPES
    std::cout << "\n4. Move-only types:\n";
    // Temporary move_only object is moved into the async call
    auto f6 = std::async(move_only());
    f6.get();
    
    // 5. LAMBDA FUNCTIONS
    std::cout << "\n5. Lambda functions:\n";
    auto lambda_task = [](int a, int b) -> int {
        std::cout << "[LAMBDA] Computing " << a << " * " << b << std::endl;
        return a * b;
    };
    
    auto f7 = std::async(lambda_task, 6, 7);
    std::cout << "Lambda result: " << f7.get() << std::endl;
}

//=============================================================================
// SECTION 3: LAUNCH POLICIES - DETAILED EXPLANATION
//=============================================================================

void demonstrate_launch_policies() {
    std::cout << "\n=== LAUNCH POLICIES DETAILED ===\n";
    
    /*
    LAUNCH POLICIES CONTROL HOW/WHEN TASKS EXECUTE:
    
    1. std::launch::async
       - MUST run on separate thread
       - Starts immediately
       - Truly asynchronous
    
    2. std::launch::deferred
       - Runs on calling thread
       - Execution deferred until get() or wait()
       - Lazy evaluation
    
    3. std::launch::deferred | std::launch::async (DEFAULT)
       - Implementation chooses
       - Usually deferred for efficiency
    
    4. No policy specified = same as (deferred | async)
    */
    
    // 1. FORCE ASYNC EXECUTION
    std::cout << "\n1. std::launch::async (forced new thread):\n";
    auto start_time = std::chrono::steady_clock::now();
    
    auto f_async = std::async(std::launch::async, []() {
        std::thread::id thread_id = std::this_thread::get_id();
        std::cout << "[ASYNC TASK] Running on thread " << thread_id << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return "async result";
    });
    
    std::thread::id main_thread_id = std::this_thread::get_id();
    std::cout << "[MAIN] Main thread " << main_thread_id << std::endl;
    std::cout << "[MAIN] Result: " << f_async.get() << std::endl;
    
    // 2. DEFERRED EXECUTION
    std::cout << "\n2. std::launch::deferred (lazy evaluation):\n";
    auto f_deferred = std::async(std::launch::deferred, []() {
        std::thread::id thread_id = std::this_thread::get_id();
        std::cout << "[DEFERRED TASK] Running on thread " << thread_id << std::endl;
        return "deferred result";
    });
    
    std::cout << "[MAIN] Deferred task created but not executed yet\n";
    std::cout << "[MAIN] Now calling get() - task will execute on this thread\n";
    std::cout << "[MAIN] Result: " << f_deferred.get() << std::endl;
    
    // 3. IMPLEMENTATION CHOICE (EXPLICIT)
    std::cout << "\n3. std::launch::deferred | std::launch::async:\n";
    auto f_choice = std::async(
        std::launch::deferred | std::launch::async,
        []() { return "implementation choice"; }
    );
    std::cout << "[MAIN] Result: " << f_choice.get() << std::endl;
    
    // 4. DEFAULT BEHAVIOR (same as above)
    std::cout << "\n4. Default behavior (no policy specified):\n";
    auto f_default = std::async([]() { return "default behavior"; });
    std::cout << "[MAIN] Result: " << f_default.get() << std::endl;
    
    // 5. PRACTICAL EXAMPLE - CHECKING IF TASK STARTED
    std::cout << "\n5. Checking if deferred task actually runs:\n";
    auto f_check = std::async(std::launch::deferred, []() {
        std::cout << "[DEFERRED] This only prints when get() is called\n";
        return 999;
    });
    
    std::cout << "[MAIN] Task created, but function hasn't run yet\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "[MAIN] Still hasn't run. Now calling get():\n";
    std::cout << "[MAIN] Result: " << f_check.get() << std::endl;
}

//=============================================================================
// SECTION 4: FUTURE OPERATIONS AND STATES
//=============================================================================

void demonstrate_future_operations() {
    std::cout << "\n=== FUTURE OPERATIONS ===\n";
    
    /*
    FUTURE OPERATIONS:
    1. get() - Blocks until ready, returns value (one-time use for std::future)
    2. wait() - Blocks until ready, doesn't return value
    3. wait_for() - Waits with timeout, returns status
    4. wait_until() - Waits until specific time point
    5. valid() - Checks if future has shared state
    */
    
    // 1. WAIT VS GET
    std::cout << "\n1. wait() vs get():\n";
    auto f1 = std::async(std::launch::async, []() {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        return 42;
    });
    
    std::cout << "[MAIN] Calling wait()...\n";
    f1.wait(); // Blocks until ready but doesn't return value
    std::cout << "[MAIN] wait() completed. Now calling get():\n";
    std::cout << "[MAIN] Result: " << f1.get() << std::endl;
    
    // 2. WAIT_FOR WITH TIMEOUT
    std::cout << "\n2. wait_for() with timeout:\n";
    auto f2 = std::async(std::launch::async, []() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return "delayed result";
    });
    
    // Try to wait for 200ms
    auto status = f2.wait_for(std::chrono::milliseconds(200));
    
    if (status == std::future_status::ready) {
        std::cout << "[MAIN] Task completed within timeout\n";
    } else if (status == std::future_status::timeout) {
        std::cout << "[MAIN] Timeout! Task still running...\n";
    } else if (status == std::future_status::deferred) {
        std::cout << "[MAIN] Task is deferred\n";
    }
    
    std::cout << "[MAIN] Final result: " << f2.get() << std::endl;
    
    // 3. CHECKING VALIDITY
    std::cout << "\n3. Future validity:\n";
    auto f3 = std::async([]() { return 100; });
    std::cout << "[MAIN] Future valid before get(): " << f3.valid() << std::endl;
    
    int result = f3.get();
    std::cout << "[MAIN] Result: " << result << std::endl;
    std::cout << "[MAIN] Future valid after get(): " << f3.valid() << std::endl;
}

//=============================================================================
// SECTION 5: std::future vs std::shared_future
//=============================================================================

void demonstrate_future_types() {
    std::cout << "\n=== std::future vs std::shared_future ===\n";
    
    /*
    DIFFERENCES:
    1. std::future<T>:
       - Unique ownership (like std::unique_ptr)
       - get() can only be called once
       - Move-only type
       - Not thread-safe for concurrent access
    
    2. std::shared_future<T>:
       - Shared ownership (like std::shared_ptr)
       - get() can be called multiple times
       - Copyable type
       - Each copy can be accessed from different threads safely
    */
    
    // 1. std::future - UNIQUE OWNERSHIP
    std::cout << "\n1. std::future (unique ownership):\n";
    {
        std::future<int> unique_fut = std::async([]() { return 42; });
        std::cout << "[MAIN] Result: " << unique_fut.get() << std::endl;
        // unique_fut.get(); // ERROR! Can't call get() twice
    }
    
    // 2. std::shared_future - SHARED OWNERSHIP
    std::cout << "\n2. std::shared_future (shared ownership):\n";
    {
        // Create shared_future from regular future
        std::future<int> temp_fut = std::async([]() { return 100; });
        std::shared_future<int> shared_fut = temp_fut.share();
        
        // Multiple threads can access the same result
        std::vector<std::thread> threads;
        
        for (int i = 0; i < 3; ++i) {
            threads.emplace_back([shared_fut, i]() {
                // Each thread gets its own copy of shared_future
                std::cout << "[THREAD " << i << "] Result: " << shared_fut.get() << std::endl;
            });
        }
        
        for (auto& t : threads) {
            t.join();
        }
        
        // Main thread can still use it
        std::cout << "[MAIN] Can still access: " << shared_fut.get() << std::endl;
    }
}

//=============================================================================
// SECTION 6: EXCEPTION HANDLING
//=============================================================================

void demonstrate_exception_handling() {
    std::cout << "\n=== EXCEPTION HANDLING ===\n";
    
    /*
    EXCEPTION HANDLING WITH FUTURES:
    - If async task throws exception, it's stored in the future
    - get() re-throws the exception on the calling thread
    - Exception is transported across thread boundaries
    - This provides clean error handling for async operations
    */
    
    // Task that throws an exception
    auto exception_task = std::async(std::launch::async, []() -> int {
        std::cout << "[ASYNC TASK] About to throw exception...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        throw std::runtime_error("Something went wrong in async task!");
        return 42; // Never reached
    });
    
    try {
        std::cout << "[MAIN] Calling get() on exception task...\n";
        int result = exception_task.get(); // Exception is re-thrown here
        std::cout << "[MAIN] Result: " << result << std::endl; // Never reached
    } catch (const std::exception& e) {
        std::cout << "[MAIN] Caught exception: " << e.what() << std::endl;
    }
}

//=============================================================================
// SECTION 7: VOID FUTURES
//=============================================================================

void demonstrate_void_futures() {
    std::cout << "\n=== VOID FUTURES ===\n";
    
    /*
    std::future<void> - FOR TASKS WITHOUT RETURN VALUES:
    - Used when you only care about completion, not return value
    - get() returns void but still blocks until completion
    - Useful for synchronization and completion notification
    */
    
    auto void_task = std::async(std::launch::async, []() {
        std::cout << "[VOID TASK] Performing work without return value...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "[VOID TASK] Work completed!\n";
        // No return statement needed
    });
    
    std::cout << "[MAIN] Waiting for void task to complete...\n";
    void_task.get(); // Returns void, but blocks until completion
    std::cout << "[MAIN] Void task completed!\n";
}

//=============================================================================
// MAIN FUNCTION
//=============================================================================

int main() {
    std::cout << "=============================================================================\n";
    std::cout << "                   COMPLETE std::async AND std::future TUTORIAL\n";
    std::cout << "=============================================================================\n";
    
    demonstrate_basic_async();
    demonstrate_async_arguments();
    demonstrate_launch_policies();
    demonstrate_future_operations();
    demonstrate_future_types();
    demonstrate_exception_handling();
    demonstrate_void_futures();
    
    std::cout << "\n=== SUMMARY ===\n";
    std::cout << "✓ std::async: Easy way to run functions asynchronously\n";
    std::cout << "✓ Arguments: Functions, member functions, lambdas, with proper syntax\n";
    std::cout << "✓ Launch policies: async (new thread), deferred (lazy), or implementation choice\n";
    std::cout << "✓ std::future: One-time access to async results\n";
    std::cout << "✓ std::shared_future: Multiple access to same async result\n";
    std::cout << "✓ Exception handling: Exceptions transported across threads via futures\n";
    std::cout << "✓ void futures: For tasks that don't return values\n";
    
    return 0;
}

