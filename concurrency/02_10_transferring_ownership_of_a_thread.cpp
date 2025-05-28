#include <iostream> // For std::cout
#include <thread>   // For std::thread
#include <chrono>   // For std::this_thread::sleep_for

// --- Functions to be executed by threads ---
void some_function() {
    std::cout << "[Thread some_function] Running..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
    std::cout << "[Thread some_function] Finished." << std::endl;
}

void some_other_function() {
    std::cout << "[Thread some_other_function] Running..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
    std::cout << "[Thread some_other_function] Finished." << std::endl;
}

int main() {
    std::cout << "--- Thread Ownership Transfer Demonstration ---" << std::endl;

    // B: Start a new thread running 'some_function' and associate it with t1.
    std::thread t1(some_function); 
    std::cout << "1. t1 is joinable: " << std::boolalpha << t1.joinable() << std::endl;

    // C: Ownership of the thread (running 'some_function') is moved from t1 to t2.
    // 'std::move()' is explicit because t1 is a named lvalue object.
    // After this, t1 no longer has an associated thread.
    std::thread t2 = std::move(t1); 
    std::cout << "2. After move from t1 to t2:" << std::endl;
    std::cout << "   t1 is joinable: " << std::boolalpha << t1.joinable() << std::endl; // Should be false
    std::cout << "   t2 is joinable: " << std::boolalpha << t2.joinable() << std::endl; // Should be true

    // D: A new temporary std::thread object is created, associated with 'some_other_function'.
    // E: Ownership of this new thread is implicitly moved into t1.
    // std::move() is NOT required because the source is a temporary (rvalue).
    t1 = std::thread(some_other_function); 
    std::cout << "3. After assigning new thread to t1:" << std::endl;
    std::cout << "   t1 is joinable: " << std::boolalpha << t1.joinable() << std::endl; // Should be true
    std::cout << "   t2 is joinable: " << std::boolalpha << t2.joinable() << std::endl; // Still true

    // F: t3 is default-constructed (no associated thread initially).
    std::thread t3; 
    std::cout << "4. t3 default constructed: " << std::boolalpha << t3.joinable() << std::endl; // Should be false

    // G: Ownership of the thread (running 'some_function', currently with t2) is moved to t3.
    // 'std::move()' is explicit because t2 is a named lvalue object.
    t3 = std::move(t2); 
    std::cout << "5. After move from t2 to t3:" << std::endl;
    std::cout << "   t1 is joinable: " << std::boolalpha << t1.joinable() << std::endl; // Still true
    std::cout << "   t2 is joinable: " << std::boolalpha << t2.joinable() << std::endl; // Should be false
    std::cout << "   t3 is joinable: " << std::boolalpha << t3.joinable() << std::endl; // Should be true

    // At this point:
    // t1 is associated with 'some_other_function'
    // t2 has no associated thread
    // t3 is associated with 'some_function'

    std::cout << "\nAttempting final move which will cause termination..." << std::endl;

    // H: DANGER! This assignment will terminate the program!
    // t1 already owns a thread (running 'some_other_function').
    // You cannot assign a new thread to a std::thread object that is already joinable
    // without explicitly joining or detaching its current thread first.
    // This rule is consistent with the std::thread destructor: you cannot just "drop" a thread.
    t1 = std::move(t3); 
    // Program will terminate here.

    // These lines will NOT be reached if the termination occurs as expected.
    std::cout << "This line will not be printed." << std::endl; 
    t1.join(); // Will not be called
    return 0;  // Will not be reached
}