#ifndef STACK_HPP
#define STACK_HPP

#include <stack>
#include <mutex>
#include <memory>
#include <exception>
#include <iostream> // Primarily for exception, but generally useful for types like std::string if used in classes.

// Custom exception for empty stack operations
struct empty_stack : std::exception {
    const char* what() const throw() {
        return "Stack is empty";
    }
};

// --- Problematic Stack Declaration ---
template<typename T>
class problematic_stack {
private:
    std::stack<T> data;
    mutable std::mutex m;

public:
    problematic_stack();
    void push(T item);
    bool empty() const;
    T top() const;
    void pop();
    size_t size() const;
};

// --- Improved Thread-Safe Stack Declaration ---
template<typename T>
class threadsafe_stack {
private:
    std::stack<T> data;
    mutable std::mutex m;

public:
    threadsafe_stack();
    threadsafe_stack(const threadsafe_stack& other);
    // Assignment operator is explicitly deleted to prevent complex concurrency issues
    threadsafe_stack& operator=(const threadsafe_stack&) = delete;
    void push(T new_value);
    std::shared_ptr<T> pop();
    void pop(T& value);
    bool empty() const;
    size_t size() const;
};

// --- Auxiliary Class for Exception Safety Demonstration Declaration ---
class ThrowingType {
private:
    int value;
    static int construction_count;

public:
    ThrowingType(int v);
    ThrowingType(const ThrowingType& other);
    ThrowingType(ThrowingType&& other) noexcept;
    ThrowingType& operator=(ThrowingType&& other) noexcept;
    ThrowingType& operator=(const ThrowingType& other);
    int get_value() const;
};

#endif // STACK_HPP