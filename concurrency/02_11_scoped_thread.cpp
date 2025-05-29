// One benefit of the move support of std::thread is that you can build on the thread_guard class from earlier
// and have it actually take ownership of the thread.
// This avoids any unpleasant consequences should the thread_guard object outlive the thread it was referencing, 
// and it also means that no one else can join or detach the thread once ownership has been transferred into the object. 
// Because this would primarily be aimed at ensuring threads are completed before a scope is exited, I named this class scoped_thread. 
#include <thread>
#include <stdexcept>
#include <iostream>

class scoped_thread 
{
    std::thread t;

public:
    explicit scoped_thread(std::thread &&t_) : t(std::move(t_)) {
        if(!t.joinable()) {
            throw std::logic_error("No thread");
        }
    }

    ~scoped_thread() {
        t.join();
    }

    scoped_thread(scoped_thread const&) = delete;
    scoped_thread& operator=(scoped_thread const &) = delete;
};


struct func {
    int& my_local_state; // Reference to local state from the calling function

    func(int& state) : my_local_state(state) {}

    void operator()() {
        std::cout << "[Thread func] Running. Local state value: " << my_local_state << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Simulate work
        my_local_state = 99; // Modify the local state (only safe if 'f' joins or waits!)
        std::cout << "[Thread func] Finished. Modified local state to: " << my_local_state << std::endl;
    }
};

void do_something_in_current_thread() {
    std::cout << "[Main Thread] Doing something else concurrently..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // Simulate work
}


void f()
{
    int some_local_state = 0;
    scoped_thread t{std::thread(func(some_local_state))};
    some_local_state = 1;
    std::thread t2{func(some_local_state)};
    scoped_thread t1(std::move(t2));
    // The parser initially considers this could be a function taking a parameter of type "function returning std::thread and taking whatever some_local_state represents as a parameter type/name".
    // The compiler only figures out later (during semantic analysis) that some_local_state is actually a variable, but by then the parse tree has already been built assuming it's a declaration.
    // This is why the Most Vexing Parse is so tricky - it's resolved at the syntax level before semantic analysis!RetryClaude does not have the ability to run the code it generates yet.Claude can make mistakes. Please double-check responses.
    do_something_in_current_thread();
    // t2.join();
}

int main() {
    f();
    return 0;
}