// If you don’t wait for your thread to finish, then you need to ensure that 
// the data accessed by the thread is valid until the thread has finished with it.

#include <iostream>
#include <thread>
#include <unistd.h>

void do_something(int i){
    std::cout <<i <<std::endl;
};

struct func
{
    int& i; 
    // If we replace the above with this : int i; the program would print everything and run as expected even with detached thread.

    func(int& i_):i(i_){}
    
    void operator()()
    {
        unsigned j = i;
        for(;j<i+5;++j)
        {
            do_something(j);
        }
        sleep(1);
        for(;j<i+10;++j)
        {
            do_something(j);
        }

    }
    // void operator()() is the overloaded function call operator (also known as the parenthesis operator) for the func struct.
    //  operator(): This is the special syntax used to overload the function call operator.
    // () (after operator): This specifies that when the object is called, it takes no arguments.
};

void oops()
{
    int some_local_state=0;
    func my_func(some_local_state);
    std::thread my_thread(my_func); // 'my_func' is copied, but its internal 'i', being a reference variable, is still a REFERENCE to 'local_state'.
    my_thread.detach(); // When 'oops()' exits, 'local_state' is destroyed, but the detached thread still tries to use 'i', leading to UNDEFINED BEHAVIOR (dangling reference).
    // Output:
    // 0
    // 1
    // 

    // On adding: 
    // sleep(2);
    // Output: 
    // rust@victus:~/cse_1/cpp/concurrency$ ./a.out
    // 0
    // 1
    // 2
    // 3
    // 4
    // 5
    // 6
    // 7
    // 8
    // 9


    // In this case, the new thread associated with my_thread will probably still be running when oops exits d, 
    // because you’ve explicitly decided not to wait for it by calling detach(). 
    // If the thread is still running, then the next call to do_something(i) will access an already destroyed variable.    

    // One common way to handle this scenario is to make the thread function self-
    // contained and copy the data into the thread rather than sharing the data. 
    // If you use a callable object for your thread function, that object is itself copied into the thread, 
    // so the original object can be destroyed immediately. 
    // But you still need to be wary of objects containing pointers or references,

    // In particular, it’s a bad idea to create a thread within a function that has access to the local variables
    // in that function, unless the thread is guaranteed to finish before the function exits.
    // Alternatively, you can ensure that the thread has completed execution before the function exits by joining with the thread.
}   

int main() {
    oops();
}


// How do I ensure parameters are copied into the thread, not passed by reference?
// By default, std::thread copies arguments.

// Pass by Value:
// If you pass a primitive type (like int, double, char) directly: std::thread t(func, my_int_var); - my_int_var will be copied.

// If you pass an object (like std::string, std::vector, or your func_by_value object) directly: std::thread t(func_by_value_obj); or std::thread t(func, my_string_var);
//  - the object (or its copy) will be copied into the thread's internal storage. This triggers the copy constructor of the object.
// Crucial caveat: If the object itself contains pointers or references to external data (like your original func struct), 
// only the pointer/reference is copied, not the underlying data. This is a shallow copy, and it leads to the dangling reference problem. 
// To truly deep copy, your object's copy constructor needs to perform a deep copy of its pointed-to/referenced data.


// Using std::ref() / std::cref():
// This explicitly tells std::thread to pass an argument by reference.
// You use this when you intend for the thread to access or modify the original variable. 
// This requires careful synchronization (e.g., with mutexes) and ensuring the original variable's lifetime exceeds the thread's execution.
// Example: std::thread t(func, std::ref(my_int_var));


// Using std::move():
// This is used when you want to transfer ownership of a unique resource (like std::unique_ptr, std::fstream, std::thread objects themselves) into the new thread.
// It doesn't copy the object, but rather moves its internal state. The original object is then typically in a valid but empty/null state.
// Example: std::thread t(func, std::move(my_unique_ptr));