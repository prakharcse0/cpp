// If one thread is waiting for a second thread to complete a task, it has several options.


// Option 1:
// It could just keep checking a flag in shared data (protected by a mutex) 
// and have the second thread set the flat when it completes the task.

// This is wasteful on two counts: 
// i.  The thread consumes valuable processing time repeatedly checking the flag.
// ii. When the mutex is locked by the waiting thread, it can't be locked by any any other thread.

// Both of these work against the thread doing the waiting, 
// because they limit the resources available to the thread being waaited for
// and even prevent it from setting the flag when it's done.

// The waiting thread is consuming resources that could be used by other 
// threads in the system and may end up waiting longer than necessary.


// Option 2:
// A second option is to have the waiting thread sleep for small periods 
// between the checks using the std::this_thread::sleep_for() function.

#include <mutex>
#include <thread>

bool flag;
std::mutex m;

void wait_for_flag() {
    std::unique_lock<std::mutex> lk(m);
    while(!flag) {
        lk.lock();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        lk.unlock();
    }
}

// In the loop, the function unlocks the mutex B before the sleep c and locks it again
// afterward d, so another thread gets a chance to acquire it and set the flag.
// This is an improvement, because the thread doesn’t waste processing time while
// it’s sleeping, but it’s hard to get the sleep period right. Too short a sleep in between
// checks and the thread still wastes processing time checking; too long a sleep and the
// thread will keep on sleeping even when the task it’s waiting for is complete, introduc-
// ing a delay. It’s rare that this oversleeping will have a direct impact on the operation of
// the program, but it could mean dropped frames in a fast-paced game or overrunning
// a time slice in a real-time application.

int main() {

    return 0;
}


// Option 3:
// The third, and preferred, option is to use the facilities from the C++ Standard
// Library to wait for the event itself. The most basic mechanism for waiting for an event
// to be triggered by another thread (such as the presence of additional work in the
// pipeline mentioned previously) is the condition variable. Conceptually, a condition vari-
// able is associated with some event or other condition, and one or more threads can wait
// for that condition to be satisfied. When some thread has determined that the condi-
// tion is satisfied, it can then notify one or more of the threads waiting on the condition
// variable, in order to wake them up and allow them to continue processing.