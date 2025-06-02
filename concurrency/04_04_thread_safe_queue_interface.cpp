#include <memory>
#include <iostream>

// Listing 4.3: The interface of threadsafe_queue
// This shows the design decisions for thread-safe queue interface

template<typename T>
class threadsafe_queue
{
public:
    // Constructors - simplified set compared to std::queue
    threadsafe_queue();
    threadsafe_queue(const threadsafe_queue&);
    
    // Assignment deleted for simplicity - avoids complex synchronization
    threadsafe_queue& operator=(const threadsafe_queue&) = delete;

    // Thread-safe push operation
    // Adds element and notifies waiting threads
    void push(T new_value);

    // Non-blocking pop operations (try_pop)
    // Returns immediately whether data is available or not
    
    // Version 1: Uses reference parameter for value, return indicates success
    bool try_pop(T& value);                    // b - status in return value
    
    // Version 2: Returns pointer to value, nullptr if no data  
    std::shared_ptr<T> try_pop();              // c - can return nullptr

    // Blocking pop operations (wait_and_pop)  
    // Waits until data is available before returning
    
    // Version 1: Sets value by reference
    void wait_and_pop(T& value);
    
    // Version 2: Returns shared_ptr to value
    std::shared_ptr<T> wait_and_pop();

    // Query operations
    bool empty() const;
    
    // Note: size() deliberately omitted - can be racy and misleading
    // in multi-threaded context
};

// Interface design explanations and demonstrations
void explain_interface_design() {
    std::cout << "=== Thread-Safe Queue Interface Design ===\n\n";
    
    std::cout << "Key Design Decisions:\n\n";
    
    std::cout << "1. COMBINED OPERATIONS:\n";
    std::cout << "   • No separate front() + pop()\n";
    std::cout << "   • Instead: try_pop() and wait_and_pop() do both atomically\n";
    std::cout << "   • Eliminates race condition window\n\n";
    
    std::cout << "2. TWO POP VARIANTS:\n";
    std::cout << "   • try_pop(): Non-blocking, returns immediately\n";
    std::cout << "   • wait_and_pop(): Blocking, waits for data\n";
    std::cout << "   • Covers different threading scenarios\n\n";
    
    std::cout << "3. DUAL RETURN METHODS:\n";
    std::cout << "   • Reference version: bool try_pop(T& value)\n";
    std::cout << "     - Uses return value for success/failure status\n";
    std::cout << "   • Pointer version: shared_ptr<T> try_pop()\n";
    std::cout << "     - Returns nullptr on failure, data pointer on success\n";
    std::cout << "     - Exception-safe with automatic memory management\n\n";
    
    std::cout << "4. SIMPLIFIED CONSTRUCTORS:\n";
    std::cout << "   • Reduced from std::queue's many constructor overloads\n";
    std::cout << "   • Assignment operator deleted - simplifies synchronization\n\n";
    
    std::cout << "5. OMITTED OPERATIONS:\n";
    std::cout << "   • No size() - can be misleading in multi-threaded context\n";
    std::cout << "   • No front()/back() - would create race conditions\n";
    std::cout << "   • Focus on operations that make sense for thread safety\n\n";
}

void demonstrate_usage_patterns() {
    std::cout << "=== Usage Patterns ===\n\n";
    
    std::cout << "PRODUCER THREAD pattern:\n";
    std::cout << "  threadsafe_queue<WorkItem> queue;\n";
    std::cout << "  while(has_work()) {\n";
    std::cout << "      WorkItem item = prepare_work();\n";
    std::cout << "      queue.push(item);              // Thread-safe push\n";
    std::cout << "  }\n\n";
    
    std::cout << "CONSUMER THREAD pattern (blocking):\n";
    std::cout << "  while(true) {\n";
    std::cout << "      WorkItem item;\n";
    std::cout << "      queue.wait_and_pop(item);      // Blocks until data available\n";
    std::cout << "      process(item);\n";
    std::cout << "  }\n\n";
    
    std::cout << "CONSUMER THREAD pattern (non-blocking):\n";
    std::cout << "  WorkItem item;\n";
    std::cout << "  if(queue.try_pop(item)) {          // Returns immediately\n";
    std::cout << "      process(item);\n";
    std::cout << "  } else {\n";
    std::cout << "      do_other_work();\n";
    std::cout << "  }\n\n";
    
    std::cout << "SHARED_PTR pattern (exception-safe):\n";
    std::cout << "  auto item_ptr = queue.try_pop();\n";
    std::cout << "  if(item_ptr) {                     // Check for nullptr\n";
    std::cout << "      process(*item_ptr);\n";
    std::cout << "  }\n\n";
}

void compare_with_standard_queue() {
    std::cout << "=== Comparison with std::queue ===\n\n";
    
    std::cout << "std::queue (NOT thread-safe):\n";
    std::cout << "  ❌ front() + pop() = race condition\n";
    std::cout << "  ❌ No blocking operations\n"; 
    std::cout << "  ❌ No built-in synchronization\n";
    std::cout << "  ✅ Many constructor options\n";
    std::cout << "  ✅ Full STL interface\n\n";
    
    std::cout << "threadsafe_queue:\n";
    std::cout << "  ✅ Atomic pop operations\n";
    std::cout << "  ✅ Blocking wait_and_pop()\n";
    std::cout << "  ✅ Built-in synchronization\n";
    std::cout << "  ✅ Exception-safe with shared_ptr\n";
    std::cout << "  ❌ Simplified interface\n";
    std::cout << "  ❌ Some operations omitted for safety\n\n";
}

int main() {
    std::cout << "Thread-Safe Queue Interface Design\n";
    std::cout << "Based on Listing 4.3 from the text\n";
    std::cout << "==================================\n\n";
    
    explain_interface_design();
    demonstrate_usage_patterns();
    compare_with_standard_queue();
    
    std::cout << "=== Next Steps ===\n";
    std::cout << "This interface addresses thread-safety issues, but needs:\n";
    std::cout << "• Mutex for synchronization\n";
    std::cout << "• Condition variables for blocking operations\n";
    std::cout << "• Proper implementation of each method\n";
    std::cout << "\nSee next examples for implementation details.\n";
    
    return 0;
}