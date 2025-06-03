#include <future>
#include <stdexcept>
#include <cmath>
#include <exception>

// ===== CORE CONCEPT: ASYNC EXCEPTION PROPAGATION =====
// Async calls store exceptions in future, rethrow on get() - same as sync behavior

double square_root(double x) {
    if(x < 0) throw std::out_of_range("x<0");
    return sqrt(x);
}

// Sync: exception thrown directly
void sync_call() {
    double y = square_root(-1);  // Throws here
}

// Async: exception stored in future, rethrown on get()
void async_call() {
    std::future<double> f = std::async(square_root, -1);
    double y = f.get();  // Exception rethrown here (original or copy - unspecified)
}

// packaged_task: same behavior as async
void packaged_task_call() {
    std::packaged_task<double(double)> task(square_root);
    std::future<double> f = task.get_future();
    task(-1);           // Exception stored in future
    double y = f.get(); // Exception rethrown here
}

// ===== PROMISE EXCEPTION STORAGE =====

double calculate_value() { return 42.0; } // Mock function

// Method 1: Store caught exception
void store_caught_exception() {
    std::promise<double> some_promise;
    try {
        some_promise.set_value(calculate_value());
    } catch(...) {
        some_promise.set_exception(std::current_exception());
    }
}

// Method 2: Store specific exception directly (preferred - cleaner & optimizable)
void store_direct_exception() {
    std::promise<double> some_promise;
    some_promise.set_exception(std::make_exception_ptr(std::logic_error("error")));
}

// ===== BROKEN PROMISE AUTO-EXCEPTION =====
// Destroying promise/packaged_task without setting value automatically stores
// std::future_error with std::future_errc::broken_promise

void broken_promise() {
    std::future<int> f;
    {
        std::promise<int> p;
        f = p.get_future();
    } // p destroyed without set_value/set_exception - broken_promise stored
    
    int result = f.get(); // Throws std::future_error
}

// ===== FUTURE LIMITATIONS =====
// std::future: only ONE thread can wait
// std::shared_future: MULTIPLE threads can wait for same result

void multiple_waiters() {
    std::shared_future<int> sf = std::async([]{ return 42; }).share();
    // Multiple threads can call sf.get() - all get same result/exception
}

/*
SUMMARY:
1. Async exception propagation: stored in future, rethrown on get()
2. Promise exception storage: set_exception() with current_exception() or make_exception_ptr()
3. Broken promise: auto-stores future_error if promise destroyed without setting
4. Future types: std::future (single waiter) vs std::shared_future (multiple waiters)
*/