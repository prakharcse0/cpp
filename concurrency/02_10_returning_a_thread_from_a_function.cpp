#include <iostream>  // For std::cout
#include <thread>    // For std::thread
#include <chrono>    // For std::this_thread::sleep_for
#include <string>    // For std::string (if needed for output)

// --- Helper Functions to be executed by threads ---
// These simulate work so we can observe thread states.
void some_function() {
    std::cout << "[Thread some_function] Running... " << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "[Thread some_function] Finished." << std::endl;
}

void some_other_function(int val) {
    std::cout << "[Thread some_other_function] Running with value: " << val << "..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::cout << "[Thread some_other_function] Finished." << std::endl;
}

// --- Listing 2.5: Returning a std::thread from a function ---
// Function f: Creates a thread and returns it.
// Return by value uses move semantics; the thread's ownership is moved out.
std::thread f_return() {
    void some_function();   // Declaration: This tells the compiler that 'actual_some_function_impl'
    // exists and what its signature is. The definition is elsewhere.
    return std::thread(some_function); // Constructs a temporary std::thread and moves it
}

// Function g: Creates a thread, then returns it.
// Return of a named variable also uses move semantics (implicit in C++11 if no copy, explicit std::move() for clarity/older compilers).
std::thread g_return() {
    // void some_other_function(int); // Already declared above
    std::thread t(some_other_function, 42); // Creates a named thread object
    return t; // t is moved out of the function
}

// --- Accepting a std::thread by value as a parameter ---
// Function f_accept: Accepts a thread by value, meaning ownership is moved into the function.
// The accepted thread 't' must be joined or detached within this function.
void f_accept(std::thread t) {
    std::cout << "[f_accept] Thread received. Joinable: " << std::boolalpha << t.joinable() << std::endl;
    if (t.joinable()) {
        t.join(); // Must join or detach to prevent std::terminate() on destructor
        std::cout << "[f_accept] Thread joined successfully." << std::endl;
    } else {
        std::cout << "[f_accept] Received non-joinable thread." << std::endl;
    }
}

// Function g_caller: Demonstrates passing threads into f_accept.
void g_caller() {
    // void some_function(); // Already declared above

    std::cout << "\n--- g_caller: Passing a temporary thread ---" << std::endl;
    // Creates a temporary std::thread object, whose ownership is implicitly moved into f_accept.
    f_accept(std::thread(some_function)); // Temporary is moved into f_accept's parameter 't'

    std::cout << "\n--- g_caller: Passing a named thread with std::move ---" << std::endl;
    std::thread t(some_function); // Creates a named thread object
    std::cout << "[g_caller] Named thread 't' created. Joinable: " << std::boolalpha << t.joinable() << std::endl;
    
    // Explicitly move ownership of 't' into f_accept.
    // After this call, 't' in g_caller will no longer be joinable.
    f_accept(std::move(t)); 
    std::cout << "[g_caller] Named thread 't' after move to f_accept. Joinable: " << std::boolalpha << t.joinable() << std::endl;
}


int main() {
    std::cout << "--- Demonstrating Returning std::thread from Functions ---" << std::endl;
    
    // Call f_return and store the returned thread in main_t1.
    // Ownership of the thread running 'some_function' is moved from f_return to main_t1.
    std::thread main_t1 = f_return();
    std::cout << "[Main] main_t1 is joinable: " << std::boolalpha << main_t1.joinable() << std::endl;

    // Call g_return and store the returned thread in main_t2.
    // Ownership of the thread running 'some_other_function' is moved from g_return to main_t2.
    std::thread main_t2 = g_return();
    std::cout << "[Main] main_t2 is joinable: " << std::boolalpha << main_t2.joinable() << std::endl;

    // Join the threads created by f_return and g_return to ensure they complete cleanly.
    if (main_t1.joinable()) {
        main_t1.join();
    }
    if (main_t2.joinable()) {
        main_t2.join();
    }

    std::cout << "\n--- Demonstrating Passing std::thread into Functions ---" << std::endl;
    g_caller(); // This function handles its own threads by passing them to f_accept.

    std::cout << "\n--- Main thread finished ---" << std::endl;
    return 0;
}