#include "stack.hpp" // Include our shared header
#include <chrono> // For std::chrono
#include <vector>   // For std::vector
#include <thread>   // For std::thread
#include <string>   // For std::string(50, '=')

// --- Declaration of Demonstration Functions ---
// These functions are defined here in stack_demos.cpp,
// but they operate on the classes declared in stack.hpp
// and implemented in problematic_stack_impl.cpp and improved_stack_impl.cpp.
void demonstrate_race_condition();
void demonstrate_safe_operations();
void demonstrate_exception_safety();
void performance_comparison();

// --- Demonstration of the Race Condition Problem ---
void demonstrate_race_condition() {
    std::cout << "\n=== DEMONSTRATING RACE CONDITION PROBLEM ===" << std::endl;
    
    problematic_stack<int> stack;
    stack.push(42);
    
    // Simulate two threads trying to access the stack
    std::thread t1([&stack]() {
        try {
            if (!stack.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Simulate delay
                // By now, thread 2 might have popped the element!
                int value = stack.top(); // POTENTIAL RACE CONDITION!
                stack.pop();
                std::cout << "Thread 1 got: " << value << std::endl;
            } else {
                std::cout << "Thread 1: Stack was empty when checked." << std::endl;
            }
        } catch (const empty_stack& e) {
            std::cout << "Thread 1 caught exception: " << e.what() << std::endl;
        }
    });
    
    std::thread t2([&stack]() {
        try {
            if (!stack.empty()) {
                int value = stack.top();
                stack.pop();
                std::cout << "Thread 2 got: " << value << std::endl;
            } else {
                std::cout << "Thread 2: Stack was empty when checked." << std::endl;
            }
        } catch (const empty_stack& e) {
            std::cout << "Thread 2 caught exception: " << e.what() << std::endl;
        }
    });
    
    t1.join();
    t2.join();
    
    std::cout << "Final stack size: " << stack.size() << std::endl;
    std::cout << std::endl;
}

// --- Demonstration of Safe Operations ---
void demonstrate_safe_operations() {
    std::cout << "\n=== DEMONSTRATING SAFE OPERATIONS ===" << std::endl;
    
    threadsafe_stack<int> safe_stack;
    
    for(int i = 1; i <= 5; ++i) {
        safe_stack.push(i * 10);
    }
    
    std::cout << "Initial stack size: " << safe_stack.size() << std::endl;
    
    std::vector<std::thread> threads;
    std::mutex output_mutex; // For safe console output
    
    for(int i = 0; i < 3; ++i) {
        threads.emplace_back([&safe_stack, &output_mutex, i]() {
            try {
                // Method 1: Pop returning shared_ptr
                auto value = safe_stack.pop();
                
                std::lock_guard<std::mutex> output_lock(output_mutex);
                std::cout << "Thread " << i << " popped (shared_ptr): " << *value << std::endl;
                
            } catch(const empty_stack& e) {
                std::lock_guard<std::mutex> output_lock(output_mutex);
                std::cout << "Thread " << i << " found empty stack" << std::endl;
            }
        });
    }
    
    for(int i = 3; i < 6; ++i) {
        threads.emplace_back([&safe_stack, &output_mutex, i]() {
            try {
                // Method 2: Pop using reference
                int value;
                safe_stack.pop(value);
                
                std::lock_guard<std::mutex> output_lock(output_mutex);
                std::cout << "Thread " << i << " popped (reference): " << value << std::endl;
                
            } catch(const empty_stack& e) {
                std::lock_guard<std::mutex> output_lock(output_mutex);
                std::cout << "Thread " << i << " found empty stack" << std::endl;
            }
        });
    }
    
    // Wait for all threads to complete
    for(auto& t : threads) {
        t.join();
    }
    
    std::cout << "Final stack size: " << safe_stack.size() << std::endl;
    std::cout << std::endl;
}

// --- Exception Safety Demonstration ---
void demonstrate_exception_safety() {
    std::cout << "\n=== DEMONSTRATING EXCEPTION SAFETY ===" << std::endl;
    
    threadsafe_stack<ThrowingType> stack;
    
    for(int i = 1; i <= 20; ++i) {
        try {
            stack.push(ThrowingType(i));
            std::cout << "Successfully pushed " << i << std::endl;
        } catch(const std::exception& e) {
            std::cout << "Failed to push " << i << ": " << e.what() << std::endl;
        }
    }
    
    std::cout << "Stack size after pushes: " << stack.size() << std::endl;
    
    while(!stack.empty()) {
        try {
            auto item = stack.pop();
            std::cout << "Popped value: " << item->get_value() << std::endl;
        } catch(const std::exception& e) {
            std::cout << "Pop failed: " << e.what() << std::endl;
            // Stack remains in consistent state even after exception
            std::cout << "Stack size after failed pop: " << stack.size() << std::endl;
        }
    }
    
    std::cout << std::endl;
}

// --- Performance Comparison ---
void performance_comparison() {
    std::cout << "\n=== PERFORMANCE COMPARISON ===" << std::endl;
    
    const int NUM_OPERATIONS = 10000;
    const int NUM_THREADS = 4;
    
    // Test problematic stack
    {
        problematic_stack<int> stack;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for(int t = 0; t < NUM_THREADS; ++t) {
            threads.emplace_back([&stack, NUM_OPERATIONS, t]() {
                for(int i = 0; i < NUM_OPERATIONS; ++i) {
                    stack.push(t * NUM_OPERATIONS + i);
                }
            });
        }
        
        for(auto& t : threads) t.join();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Problematic stack push operations: " << duration.count() 
                  << "ms (final size: " << stack.size() << ")" << std::endl;
    }
    
    // Test safe stack
    {
        threadsafe_stack<int> stack;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for(int t = 0; t < NUM_THREADS; ++t) {
            threads.emplace_back([&stack, NUM_OPERATIONS, t]() {
                for(int i = 0; i < NUM_OPERATIONS; ++i) {
                    stack.push(t * NUM_OPERATIONS + i);
                }
            });
        }
        
        for(auto& t : threads) t.join();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Thread-safe stack push operations: " << duration.count() 
                  << "ms (final size: " << stack.size() << ")" << std::endl;
    }
}

int main() {
    std::cout << "THREAD-SAFE STACK DEMONSTRATIONS\n" << std::string(50, '=') << std::endl;
    
    demonstrate_race_condition();
    demonstrate_safe_operations();
    demonstrate_exception_safety();
    performance_comparison();
    
    return 0;
}