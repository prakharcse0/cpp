#include <queue>
#include <iostream>
#include <string>

// Demonstration of std::queue interface from Listing 4.2
// Shows the standard operations and why they're problematic in multi-threaded code

template <class T, class Container = std::deque<T>>
class standard_queue_demo {
public:
    // Constructor examples (simplified for demo)
    explicit standard_queue_demo() {}
    
    // Core operations that std::queue provides
    bool empty() const { return q.empty(); }
    size_t size() const { return q.size(); }
    
    // Access operations - PROBLEMATIC in multi-threaded code
    T& front() { return q.front(); }
    const T& front() const { return q.front(); }
    T& back() { return q.back(); }
    const T& back() const { return q.back(); }
    
    // Modification operations
    void push(const T& x) { q.push(x); }
    void push(T&& x) { q.push(std::move(x)); }
    void pop() { q.pop(); }  // Note: pop() doesn't return the value!
    
private:
    std::queue<T, Container> q;
};

// Helper function to demonstrate the race condition problem
void demonstrate_race_condition_problem() {
    std::cout << "=== Race Condition Problem with Standard Queue ===\n";
    std::cout << "Problem: Between front() and pop(), another thread could modify queue\n\n";
    
    std::queue<std::string> unsafe_queue;
    unsafe_queue.push("First");
    unsafe_queue.push("Second");
    
    std::cout << "Unsafe pattern (problematic in multi-threaded code):\n";
    std::cout << "1. if (!queue.empty())           // Thread A checks\n";
    std::cout << "2.     value = queue.front();    // Thread B might pop() here!\n";
    std::cout << "3.     queue.pop();              // Thread A tries to pop\n";
    std::cout << "\nResult: Race condition! Thread A might access invalid data.\n\n";

    // Safe single-threaded usage
    std::cout << "Single-threaded usage (safe):\n";
    while (!unsafe_queue.empty()) {
        std::cout << "Front: " << unsafe_queue.front() << std::endl;
        unsafe_queue.pop();
    }
}

// Demonstrate the three groups of operations mentioned in the text
void demonstrate_operation_groups() {
    std::cout << "\n=== Three Groups of Queue Operations ===\n";
    
    standard_queue_demo<int> demo_queue;
    
    // Add some data
    demo_queue.push(10);
    demo_queue.push(20);
    demo_queue.push(30);
    
    std::cout << "1. Query whole queue state:\n";
    std::cout << "   empty(): " << (demo_queue.empty() ? "true" : "false") << std::endl;
    std::cout << "   size(): " << demo_queue.size() << std::endl;
    
    std::cout << "\n2. Query queue elements:\n";
    std::cout << "   front(): " << demo_queue.front() << std::endl;
    std::cout << "   back(): " << demo_queue.back() << std::endl;
    
    std::cout << "\n3. Modify queue:\n";
    std::cout << "   push(40) - adding element\n";
    demo_queue.push(40);
    std::cout << "   pop() - removing front element\n";
    demo_queue.pop();
    std::cout << "   New front: " << demo_queue.front() << std::endl;
    
    std::cout << "\nProblem: In multi-threaded code, these separate operations\n";
    std::cout << "create race conditions between threads!\n";
}

int main() {
    std::cout << "Standard Queue Interface Analysis\n";
    std::cout << "Based on std::queue<> container adaptor\n";
    std::cout << "========================================\n\n";
    
    demonstrate_race_condition_problem();
    demonstrate_operation_groups();
    
    std::cout << "\n=== Key Insights ===\n";
    std::cout << "• std::queue separates front() and pop() operations\n";
    std::cout << "• This separation creates race conditions in multi-threaded code\n";
    std::cout << "• Need to combine operations atomically for thread safety\n";
    std::cout << "• Solution: Create thread-safe variants that combine operations\n";
    
    return 0;
}