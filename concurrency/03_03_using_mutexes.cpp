// Synchronization primitive called a mutex (mutual exclusion).

// Before accessing a shared data structure, you lock the mutex associated with that data, 
// and when you’ve finished accessing the data structure, you unlock the mutex. 

// The Thread Library then ensures that once one thread has locked a specific mutex, 
// all other threads that try to lock the same mutex have to wait 
// until the thread that successfully locked the mutex unlocks it. 

// This ensures that all threads see a self-consistent view of the shared data, 
// without any broken invariants.

#include <mutex>
#include <list>
#include <algorithm>
// In C++, you create a mutex by constructing an instance of std::mutex, 
// lock it with a call to the member function lock(), 
// and unlock it with a call to the member function unlock(). 

// However, it isn’t recommended practice to call the member functions directly, 
// because this means that you have to remember to call unlock() on every code path out of a function, 
// including those due to exceptions. 

// Instead, the Standard C++ Library provides the std::lock_guard class template, 
// which implements that RAII idiom for a mutex; 
// it locks the supplied mutex on construction and unlocks it on destruction, 
// thus ensuring a locked mutex is always correctly unlocked. 

// The following listing shows how to protect a list that can be accessed by multiple threads
// using a std::mutex, along with std::lock_guard.

// Both of these are declared in the <mutex> header.

std::list<int> some_list;
std::mutex some_mutex;

void add_to_list(int new_value) {
    std::lock_guard<std::mutex> guard(some_mutex);
    some_list.push_back(new_value);
}

bool list_contains(int value_to_find) {
    std::lock_guard<std::mutex> gaurd(some_mutex);
    return std::find(some_list.begin(), some_list.end(), value_to_find) != some_list.end();
}


int main() {
    
    return 0;
}


// Although there are occasions where this use of global variables is appropriate, 
// in the majority of cases it’s common to group the mutex and the protected data together
// in a class rather than use global variables. 

// This is a standard application of object-oriented design rules: 
// by putting them in a class, you’re clearly marking them as related, 
// and you can encapsulate the functionality and enforce the protection. 

// In this case, the functions add_to_list and list_contains would become member functions of the class, 
// and the mutex and protected data would both become private members of the class, 
// making it much easier to identify which code has access to the data and thus which code needs to lock the mutex. 

// If all the member functions of the class lock the mutex before accessing any other data members 
// and unlock it when done, the data is nicely protected from all comers.

// Well, that’s not quite true : if one of the member functions returns a pointer or reference to the protected data, 
// then it doesn’t matter that the member functions all lock the mutex in a nice orderly fashion,
// because you’ve just blown a big hole in the protection. 
// Any code that has access to that pointer or reference can now access (and potentially modify) 
// the protected data without locking the mutex. 
// Protecting data with a mutex therefore requires careful interface design, 
// to ensure that the mutex is locked before there’s any access to the protected data and
// that there are no backdoors.