// Write code to:

// Declare an empty std::unique_ptr that will manage an int.
// Create a std::unique_ptr named myIntPtr that owns a dynamically allocated int with the value 10. Use the preferred C++14+ method.
// Access and print the value managed by myIntPtr.
// Check if myIntPtr is currently owning an object and print a message accordingly.

#include <iostream>
#include <memory>

int main() {
    std::unique_ptr<int> myIntPtr(std::make_unique<int>(10)); // How to check for error here, if the pre was initiaised or not ?
    // or: // std::unique_ptr<int> myIntPtr = std::make_unique<int>(10);
    std::cout <<*myIntPtr <<std::endl;
    if(myIntPtr) 
        std::cout <<"myIntPtr owns an object" <<std::endl;
    else
        std::cout <<"myIntPtr doesn't own an object" <<std::endl;

    return 0;
}

// std::make_unique will always return an initialized std::unique_ptr. 
// It will either own the newly allocated object, or, if the underlying new allocation fails, it will throw an exception (specifically std::bad_alloc).
// It does NOT return an "uninitialized" unique_ptr or a null unique_ptr on allocation failure. 
// If new throws std::bad_alloc, the std::make_unique call itself will terminate by throwing.

// If an exception is thrown after a std::unique_ptr has successfully acquired a resource (i.e., it has been initialized and owns an object), 
// that resource will be automatically and safely released when the std::unique_ptr goes out of scope due to stack unwinding. 
// This prevents memory leaks and other resource leaks.

