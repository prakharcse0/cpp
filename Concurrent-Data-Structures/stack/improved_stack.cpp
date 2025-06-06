#include "stack.hpp" // Include our shared header
#include <utility> // For std::move

// --- Improved Thread-Safe Stack Definition ---
template<typename T>
threadsafe_stack<T>::threadsafe_stack() {}

template<typename T>
threadsafe_stack<T>::threadsafe_stack(const threadsafe_stack& other) {
    std::lock_guard<std::mutex> lock(other.m);
    data = other.data;
}

template<typename T>
void threadsafe_stack<T>::push(T new_value) {
    std::lock_guard<std::mutex> lock(m);
    data.push(std::move(new_value));
}

template<typename T>
std::shared_ptr<T> threadsafe_stack<T>::pop() {
    std::lock_guard<std::mutex> lock(m);
    
    if(data.empty()) throw empty_stack();
    
    std::shared_ptr<T> const res(
        std::make_shared<T>(std::move(data.top()))
    );
    
    data.pop();
    return res;
}

template<typename T>
void threadsafe_stack<T>::pop(T& value) {
    std::lock_guard<std::mutex> lock(m);
    
    if(data.empty()) throw empty_stack();
    
    value = std::move(data.top());
    data.pop();
}

template<typename T>
bool threadsafe_stack<T>::empty() const {
    std::lock_guard<std::mutex> lock(m);
    return data.empty();
}

template<typename T>
size_t threadsafe_stack<T>::size() const {
    std::lock_guard<std::mutex> lock(m);
    return data.size();
}

// --- Auxiliary Class for Exception Safety Demonstration Definition ---
int ThrowingType::construction_count = 0; // Initialize static member

ThrowingType::ThrowingType(int v) : value(v) {
    construction_count++;
    if(construction_count % 7 == 0) {
        throw std::runtime_error("Construction failed");
    }
}

ThrowingType::ThrowingType(const ThrowingType& other) : value(other.value) {
    construction_count++;
    if(construction_count % 5 == 0) {
        throw std::runtime_error("Copy construction failed");
    }
}

ThrowingType::ThrowingType(ThrowingType&& other) noexcept : value(other.value) {
    // No throw from move constructor for this example
}

ThrowingType& ThrowingType::operator=(ThrowingType&& other) noexcept {
    if (this != &other) {
        value = other.value;
    }
    return *this;
}

ThrowingType& ThrowingType::operator=(const ThrowingType& other) {
    if (this != &other) {
        if(construction_count % 3 == 0) { // Using a different condition for variety
            throw std::runtime_error("Copy assignment failed");
        }
        value = other.value;
    }
    return *this;
}

int ThrowingType::get_value() const { return value; }

// Explicit instantiations for threadsafe_stack<int> and threadsafe_stack<ThrowingType>
// This ensures the compiler generates the code for these types,
// which will be used in stack_demos.cpp.
template class threadsafe_stack<int>;
template class threadsafe_stack<ThrowingType>;