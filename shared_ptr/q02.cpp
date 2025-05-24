// Challenge 2: Custom Deleters & Raw Pointer Interaction

// Task:
// You are working with a legacy C-style API that manages FILE pointers.

// 1. Define a custom deleter (as a lambda or a function) that closes a FILE* using fclose() and prints "File closed by custom deleter!".

// 2. In main, open a file (e.g., "log.txt") for writing using fopen(). Handle potential nullptr return from fopen().

// 3. Create a std::shared_ptr<FILE> that manages this FILE* using your custom deleter. 
// Write some text to the file using fprintf() and the raw pointer obtained via get().

// 4. Critical Question: Explain why it would be a severe error to call fclose(file_ptr.get()) directly after using get(), even if file_ptr is still in scope. 
// What specific problem would this cause?

// Concepts Tested: Custom deleters (std::shared_ptr constructor with deleter), interaction with raw pointers, 
// get(), danger of manual delete/fclose on managed resources, double deletion.

#include <cstdio>
#include <iostream>
#include <memory>

auto fileDeleter = [](FILE *file) {
    std::cout <<"File deletor activated: ";
    if(!file) {
        std::cout <<"File pointing to nullptpr" <<std::endl;
        return;
    }
    std::cout <<"Calling fclose() to close the file" <<std::endl;
    fclose(file);
};

int main() {
    // Alternatively:
    // FILE* rawFilePtr = fopen("log.txt", "w");
    // if (rawFilePtr == nullptr) {
    //     std::cerr << "Error: Could not open 'log.txt' for writing." << std::endl;
    //     return 1; // Indicate an error
    // }
    // std::cout << "File 'log.txt' opened successfully. Raw pointer: " << rawFilePtr << std::endl;
    // std::shared_ptr<FILE> file_ptr(rawFilePtr, fileDeleter);
    // std::cout <<"file_ptr.use_count(): " <<file_ptr.use_count() <<std::endl;

    std::shared_ptr<FILE> file_ptr(fopen("log.txt", "w"), fileDeleter);

    if (file_ptr) // Check if shared_ptr successfully manages a file
        fprintf(file_ptr.get(), "Hello from modern C++ using shared_ptr!\n");
    else
        std::cerr << "Error: shared_ptr is null after initialization." << std::endl;

    std::cout << "End of main function. shared_ptr will go out of scope now." << std::endl;

    return 0; // Program ends, file_ptr goes out of scope and calls its deleter.
}