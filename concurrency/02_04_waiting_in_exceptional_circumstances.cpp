// As mentioned earlier, you need to ensure that you’ve called either join() 
// or detach() before a std::thread object is destroyed. 

// If you’re detaching a thread, you can usually call detach() immediately after the thread has been started, 
// so this isn’t a problem. 

// But if you’re intending to wait for the thread, you need to pick carefully the
// place in the code where you call join(). 

// This means that the call to join() is liable to be skipped 
// if an exception is thrown after the thread has been started but before the call to join().

// To avoid your application being terminated when an exception is thrown, you
// therefore need to make a decision on what to do in this case. 

// In general, if you were intending to call join() in the non-exceptional case, 
// you also need to call join() in the presence of an exception to avoid accidental lifetime problems. 
#include <iostream>
#include <thread>

void do_something(int i){
    std::cout << "Thread ID: " << std::this_thread::get_id() << " - Value: " << i << std::endl;
};

struct func
{
    int& i; 

    func(int& i_):i(i_){}
    
    void operator()()
    {
        for(unsigned j=i;j<i+10;++j)
        {
            do_something(j);
        }
    }
};

void do_something_in_current_thread() {
    std::cout << "Main thread (ID: " << std::this_thread::get_id() << ") is about to throw an exception!\n";
    throw std::runtime_error("An intentional error occurred in the main thread!");
}


void f()
{
    int some_local_state=0;
    func my_func(some_local_state);
    std::thread t(my_func);
    try
    {
        do_something_in_current_thread();
    }
    catch (...) // Catch any type of exception
    {
        // std::cout << "Caught an exception in f(). Ensuring thread 't' is joined before re-throwing.\n";
        t.join(); // Join the thread to wait for it to complete
        throw;    // Re-throw the original exception to propagate it up the call stack
    }

    // If no exception was thrown, execution continues here
    std::cout << "No exception occurred in f(). Joining thread 't' normally.\n";
    t.join(); // Join the thread to wait for it to complete
}


int main() {
    f();
}