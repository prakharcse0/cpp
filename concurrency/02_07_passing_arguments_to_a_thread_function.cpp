// Passing arguments to the callable object or function is fundamentally as 
// simple as passing additional arguments to the std::thread constructor.

// But it’s important to bear in mind that by default the arguments are copied into internal storage, 
// where they can be accessed by the newly created thread of execution,
// even if the corresponding parameter in the function is expecting a reference.

#include <iostream>
#include <thread>

void f(int i,std::string const& s) {
    std::cout <<s <<std::endl;
}

int main() {
    // somethings bout strings:

    // char *str = "Hello";

    // str[1] = 'i';    // Segmentation fault (core dumped)

    // std::string &str1 = str;
    // error: cannot bind non-const lvalue reference of type ‘std::string&’ {aka ‘std::__cxx11::basic_string<char>&’} to an rvalue of type ‘std::string’ {aka ‘std::__cxx11::basic_string<char>’}

    // const std::string &str1 = str; // valid
    // A const lvalue reference (const std::string &str1) can bind to a temporary object, extending its lifetime to that of the reference.
    // So, str1 will reference this temporary std::string object which contains a deep copy of "Hello".


    std::thread t(f, 3, "hello");
    t.detach();

    // This creates a new thread of execution associated with t, which calls f(3,”hello”).
    // Note that even though f takes a std::string as the second parameter, 
    // the string literal is passed as a char const* 
    // and converted to a std::string only in the context of the new thread.

    // "hello": This is a C-style string literal. It's an array of chars (specifically const char[6] including the null terminator) with static storage duration. 
    // This means its contents ('h', 'e', 'l', 'l', 'o', '\0') reside in a read-only part of your program's memory for the entire program's lifetime. 
    // When you refer to "hello", you're getting a const char* pointer to the first character of this array.

    // std::thread t(f, 3, "hello");:
    // The std::thread constructor takes the const char* pointer (which points to the literal "hello") 
    // and copies this const char* pointer value into its internal storage. 
    // The pointer is copied, not the characters themselves.
    // This copied const char* pointer is held by the std::thread object.
    
    // When f is called in the new thread:
    // The std::string const& s parameter of f needs a std::string object.
    // Since the argument passed to f by std::thread is a const char* (the copied pointer to "hello"), 
    // the compiler invokes the std::string constructor that takes a const char* (i.e., std::string::string(const char*)).
    // This std::string constructor will then read the characters from where the const char* points ("hello" in static memory) 
    // and allocate new memory on the heap to store its own independent copy of these characters. This is a deep copy operation.
    // The std::string const& s parameter then references this newly created std::string object.


    char buffer[1024];
    sprintf(buffer, "%i",42);
    // std::thread t1(f,3,buffer);

    // In this case, it’s the pointer to the local variable buffer B that’s passed through to the new thread c, 
    // and there’s a significant chance that the function oops will exit 
    // before the buffer has been converted to a std::string on the new thread, 
    // thus leading to undefined behavior.
    // The solution is to cast to std::string before passing the buffer to the std::thread constructor:
    std::thread t1(f,3,std::string(buffer));
    t1.detach();

    /*
     * BEHAVIOR OF STRING/BUFFER ARGUMENTS IN std::thread:
     *
     * Problematic line:
     * std::thread t1(f, 3, buffer); // (B) Pass a pointer to the local buffer
     *
     * 1. 'buffer' (char[]) decays to a 'char*' (pointer to its first element).
     * 2. std::thread COPIES THIS 'char*' POINTER VALUE internally.
     * It DOES NOT copy the contents of 'buffer' at this stage.
     * 3. 't1.detach()' allows 'oops()' to exit immediately.
     * 4. When 'oops()' exits, 'buffer' (a local variable) is DESTROYED.
     * The memory it occupied becomes invalid. (C)
     * 5. The detached thread 't1' later tries to call 'f'.
     * The 'std::string const& s' parameter in 'f' causes an implicit
     * conversion from 'char*' to 'std::string'.
     * 6. This conversion attempts to read from the original (now invalid)
     * memory location that 'buffer' used to point to.
     * 7. Result: DANGLING POINTER / USE-AFTER-FREE -> UNDEFINED BEHAVIOR.
     *
     * Solution: Explicitly convert to std::string *before* passing to std::thread.
     * This ensures the actual string content is deep-copied into std::thread's
     * internal storage, guaranteeing its validity for the thread's lifetime.
     */


    // It’s also possible to get the reverse scenario: the object is copied, and what you
    // wanted was a reference. This might happen if the thread is updating a data structure
    // that’s passed in by reference, for example:

    return 0;
}
