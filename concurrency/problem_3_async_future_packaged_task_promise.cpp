// Concept Focus:

// std::async: Easiest way to run an async task and get a future.
// std::future: Retrieve results (or exceptions) from async operations.
// std::packaged_task: Wrap a callable to make its return value available via a future.
// std::promise: Set a value (or exception) that can be retrieved via a future at a later time.

// Scenario: You need to perform several asynchronous computations and retrieve their results. 
// Some are simple function calls, others involve setting results from different parts of your code.

// Task:

// 1.  Using std::async:
//     Create a function int calculate_sum(int a, int b) that returns a + b.
//     Use std::async to call this function and get its std::future<int>.
//     Retrieve and print the result.

// 2.  Using std::packaged_task:
//     Create a function std::string reverse_string(std::string s) that reverses a string.
//     Wrap this function in a std::packaged_task<std::string(std::string)>.
//     Get its std::future<std::string>.
//     Launch a new std::thread to execute the packaged_task.
//     Retrieve and print the result.

// 3.  Using std::promise:
//     Create a function void producer_function(std::promise<double> p) that 
//     performs some work and eventually sets a double value into the promise.
//     In main, create a std::promise<double> and get its associated std::future<double>.
//     Launch a std::thread to run producer_function with the promise.
//     Wait for and print the double result from the future.
//     Demonstrate exception handling: Modify producer_function to sometimes throw an exception, 
//     and show how std::future::get() rethrows it.


#include <iostream>
#include <future> // For std::async, std::future, std::packaged_task, std::promise
#include <thread>
#include <string>
#include <vector>
#include <algorithm> // For std::reverse
#include <chrono>
#include <stdexcept> // For std::runtime_error

// --- 1. Using std::async ---
int calculate_sum(int a, int b) {
    std::cout << "[calculate_sum] Calculating sum of " << a << " and " << b << " in thread: " << std::this_thread::get_id() << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
    return a + b;
}

// --- 2. Using std::packaged_task ---
std::string reverse_string(std::string s) {
    std::cout << "[reverse_string] Reversing string '" << s << "' in thread: " << std::this_thread::get_id() << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(150)); // Simulate work
    std::reverse(s.begin(), s.end());
    return s;
}

// --- 3. Using std::promise ---
void producer_function(std::promise<double> p) {
    try {
        std::cout << "[producer_function] Working... in thread: " << std::this_thread::get_id() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // Simulate work

        // TODO: Occasionally throw an exception (e.g., if random condition met)
        if (rand() % 2 == 0) throw std::runtime_error("Simulated error in producer!");

        double result = 3.14159; // The value to be set
        p.set_value(result); // Set the result
        std::cout << "[producer_function] Value set: " << result << std::endl;
    } catch (...) {
        // Catch any exception and set it in the promise
        p.set_exception(std::current_exception());
        std::cerr << "[producer_function] Caught exception and set it in promise.\n";
    }
}


int main() {
    // --- 1. std::async demonstration ---
    std::cout << "--- std::async Demonstration ---\n";
    // TODO: Launch calculate_sum asynchronously and get its future
    // std::future<int> future_sum = std::async(...);

    // TODO: Get and print the result
    // try {
    //     int sum_result = future_sum.get();
    //     std::cout << "Async sum result: " << sum_result << std::endl;
    // } catch (const std::exception& e) {
    //     std::cerr << "Error in async sum: " << e.what() << std::endl;
    // }

    std::future<int> future_sum = std::async(calculate_sum, a, b);
    try {
        int sum_result = future_sum.get();
        std::cout << "Async sum result: " << sum_result << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error in async sum: " << e.what() << std::endl;
    }

    // --- 2. std::packaged_task demonstration ---
    std::cout << "\n--- std::packaged_task Demonstration ---\n";
    // TODO: Create a packaged_task for reverse_string
    // std::packaged_task<std::string(std::string)> task(reverse_string);
    std::packaged_task<std::string (std::string)> task(reverse_string);

    // TODO: Get its future
    // std::future<std::string> future_reversed_string = task.get_future();
    std::future<std::string> future_reversed_string = task.get_future();

    // TODO: Launch a thread to run the packaged_task
    std::thread t(std::move(task), "hello world");

    // TODO: Get and print the result
    try {
        std::string reversed_str = future_reversed_string.get();
        std::cout << "Packaged task reversed string: '" << reversed_str << "'\n";
    } catch (const std::exception& e) {
        std::cerr << "Error in packaged task: " << e.what() << std::endl;
    }
    t.join(); // Join the thread


    // --- 3. std::promise demonstration ---
    std::cout << "\n--- std::promise Demonstration ---\n";
    // TODO: Create a promise and get its future
    std::promise<double> promise_double;
    std::future<double> future_double = promise_double.get_future();

    // TODO: Launch a thread to run producer_function with the promise
    std::thread prod_t(producer_function, std::move(promise_double));

    // TODO: Get and print the result (with exception handling)
    try {
        double val = future_double.get();
        std::cout << "Promise value received: " << val << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception received from promise: " << e.what() << std::endl;
    }
    prod_t.join();


    return 0;
}