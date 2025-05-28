// The use of try/catch blocks is verbose, 
// and it’s easy to get the scope slightly wrong, 
// so this isn’t an ideal scenario. 

// If it’s important to ensure that
// the thread must complete before the function exits 
// —whether because it has a reference to other local variables 
// or for any other reason

// —then it’s important to ensure
// this is the case for all possible exit paths, whether normal or exceptional, 
// and it’s desirable to provide a simple, concise mechanism for doing so.

// One way of doing this is to use the standard Resource Acquisition Is Initialization
// (RAII) idiom and provide a class that does the join() in its destructor, as in the following listing.

// RAII (Resource Acquisition Is Initialization) is a fundamental C++ programming technique where resource management is tied to object lifetime. 
// The core idea is that resources are acquired in a constructor and automatically released in the destructor.

#include <iostream>
#include <thread>
#include <chrono>

class thread_guard 
{
    std::thread &t;
public:
    explicit thread_guard(std::thread &t_): t(t_) {};
    // In C++ OOP, the explicit keyword is a specifier that can be applied to constructors and conversion operators to prevent implicit (automatic) type conversions.

    ~thread_guard() {
        if(t.joinable()) {
            t.join();
        }
    }

    thread_guard(thread_guard const&) = delete;
    thread_guard& operator=(thread_guard const&) = delete;
    // = delete;: This is a C++11 feature (and later) that explicitly tells the compiler: "Do not generate or allow this special member function."
    // These lines explicitly delete the copy constructor and the copy assignment operator for the thread_guard class.
};

void some_func() {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout <<"Ran some func" <<std::endl;
}

void f()
{
    std::thread t(some_func);
    thread_guard g(t);
}

int main() {
    f();
}