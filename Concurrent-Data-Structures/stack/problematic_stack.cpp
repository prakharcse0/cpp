#include "stack.hpp" // Include our shared header

// --- Problematic Stack Definition ---
template<typename T>
problematic_stack<T>::problematic_stack() {}

template<typename T>
void problematic_stack<T>::push(T item) {
    std::lock_guard<std::mutex> lock(m);
    data.push(item);
}

template<typename T>
bool problematic_stack<T>::empty() const {
    std::lock_guard<std::mutex> lock(m);
    return data.empty();
}

template<typename T>
T problematic_stack<T>::top() const {
    std::lock_guard<std::mutex> lock(m);
    if(data.empty()) throw empty_stack();
    return data.top();
}

template<typename T>
void problematic_stack<T>::pop() {
    std::lock_guard<std::mutex> lock(m);
    if(data.empty()) throw empty_stack();
    data.pop();
}

template<typename T>
size_t problematic_stack<T>::size() const {
    std::lock_guard<std::mutex> lock(m);
    return data.size();
}

// Explicit instantiation for problematic_stack<int>
// This ensures the compiler generates the code for 'int' type,
// which will be used in stack_demos.cpp.
template class problematic_stack<int>;