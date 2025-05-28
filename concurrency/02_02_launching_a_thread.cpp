// Every C++ program has at least one thread, which is started by the C++ runtime: the thread running main(). 

// Your program can then launch additional threads that have another function as the entry point.

// These threads then run concurrently with each other and with the initial thread. 

// Just as the program exits when the program returns from main(), 
// when the specified entry point function returns, the thread exits. 

// If you have a std::thread object for a thread, you can wait for it to finish;
// but first you have to start it, so let’s look at launching threads.

#include <iostream>
#include <thread>
#include <cassert>

void do_some_work() {
    std::cout <<"doing some work" <<std::endl;
}
void do_something() {
    std::cout <<"doing something" <<std::endl;
}
void do_something_else() {
    std::cout <<"doing something else" <<std::endl;
}

class background_task
{
public:
    void operator()() const
    {
        do_something();
        do_something_else();
    }
};


int main() {

    // 1. A simple thread launch: 
    std::thread my_thread(do_some_work);
    // Output: doing some work
    my_thread.join();
    // Output if U comment this join statements:
    // terminate called without an active exception
    // Aborted (core dumped)

    // The act of calling join() also cleans up any storage associated with the thread, 
    // so the std::thread object is no longer associated with the now-finished thread; 
    // it isn’t associated with any thread. 
    // This means that you can call join() only once for a given thread; 
    // once you’ve called join(), the std::thread object is no longer joinable, 
    // and joinable() will return false.
    assert(!my_thread.joinable());
    // If I did this intead: assert(my_thread.joinable());
    // Full Output: 
    // doing some work
    // a.out: 01_02_launching_a_thread.cpp:54: int main(): Assertion `my_thread.joinable()' failed.
    // Aborted (core dumped)


    // 2. As with much of the C++ Standard Library, std::thread works with any callable type, 
    // so you can pass an instance of a class with a function call operator to the std::thread constructor instead:
    background_task f;
    std::thread my_thread_1(f);
    // Output: doing something
    //         doing something else
    my_thread_1.join();


    // 3. “C++’s most vexing parse” :
    // If you pass a temporary rather than a named variable, then the syntax can be the same as that of a 
    // function declaration, in which case the compiler interprets it as such, rather than an object definition.
    // For example,
    std::thread my_thread_2(background_task());
    // Output: 
    // Warning : 01_02_launching_a_thread.cpp: In function ‘int main()’:
    // 01_02_launching_a_thread.cpp:59:28: warning: parentheses were disambiguated as a function declaration [-Wvexing-parse]
    //          std::thread my_thread_2(background_task());

    // std::thread my_thread(background_task());
    // declares a function my_thread that takes a single parameter 
    // (of type pointer to a function taking no parameters and returning a background_task object) 
    // and returns a std::thread object, 
    // rather than launching a new thread. 
    // You can avoid this by naming your function object as shown previously, 
    // by using an extra set of parentheses, or
    // by using the new uniform initialization syntax, 
    // for example:
    std::thread my_thread_3((background_task()));  // the extra parentheses prevent interpretation as a function declaration, thus allowing my_thread to be declared as a variable of type std::thread.
    std::thread my_thread_4{background_task()};  // the new uniform initialization syntax with braces rather than parentheses, and thus would also declare a variable.
    // Output: 
    // doing something
    // doing something else
    // doing something
    // doing something else
    my_thread_3.join();
    my_thread_4.join();


    // 4. lambdas:
    // One type of callable object that avoids this problem is a lambda expression.
    // The previous example can be written using a lambda expression as follows:
    std::thread my_thread_5([](){
        do_something();
        do_something_else();
    });
    // Output:
    // doing something
    // doing something else

    my_thread_5.join();

    // Once you’ve started your thread, you need to explicitly decide whether to wait for it to
    // finish (by joining with it) or leave it to run on its own (by detaching it). 

    // If you don’t decide before the std::thread object is destroyed,
    // then your program is terminated (the std::thread destructor calls std::terminate()).
    
    // It’s therefore imperative that you ensure that the thread is correctly joined or
    // detached, even in the presence of exceptions. 
    
    // Note that you only have to make this decision before the std::thread 
    // object is destroyed — the thread itself may well have finished long before you join with
    
    // it or detach it, and if you detach it, then the thread may continue running long after
    // the std::thread object is destroyed.
    return 0;
}