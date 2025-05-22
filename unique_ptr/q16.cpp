// Coding: You have a C-style file handle FILE* obtained from fopen(). This handle needs to be closed with fclose().
// Define a custom deleter (e.g., a lambda or a functor) that calls fclose() on a FILE*.
// Create a std::unique_ptr to manage a FILE* for writing to "my_log.txt", using your custom deleter.
// Write some text to the file using the unique_ptr's managed pointer (get()).
// Observe the file being closed automatically when the unique_ptr goes out of scope. (Don't worry about file contents, just the fclose call).

#include <iostream>  // For std::cout, std::cerr
#include <memory>    // For std::unique_ptr
#include <cstdio>    // For FILE, fopen, fprintf, fclose

// 1. Define a custom deleter using a lambda expression
// A stateless lambda is generally preferred for its conciseness.
auto fileDeleter = [](FILE* file) {
    if (file) { // Always check if the pointer is not null before attempting to close
        std::cout << "Custom deleter activated: Calling fclose() to close the file." << std::endl;
        fclose(file);
    } else {
        std::cout << "Custom deleter: Received a nullptr, no file to close." << std::endl;
    }
};

int main() {
    std::cout << "Main function started." << std::endl;

    // 2. Create a std::unique_ptr to manage a FILE* for writing to "my_log.txt",
    //    using your custom deleter.
    // The type of unique_ptr needs to specify both the managed type (FILE*)
    // and the type of the deleter (decltype(fileDeleter) for our lambda).
    std::unique_ptr<FILE, decltype(fileDeleter)> logFilePtr(
        fopen("my_log.txt", "w"), // Acquire the resource (open file for writing)
        fileDeleter               // Provide the custom deleter
    );

    // Check if fopen was successful
    if (logFilePtr) {
        std::cout << "File 'my_log.txt' opened successfully." << std::endl;

        // 3. Write some text to the file using the unique_ptr's managed pointer (get()).
        fprintf(logFilePtr.get(), "Log entry: Application started at %s.\n", __TIME__);
        fprintf(logFilePtr.get(), "Log entry: Performing some operations...\n");
        std::cout << "Text written to 'my_log.txt'." << std::endl;

    } else {
        std::cerr << "Error: Could not open 'my_log.txt' for writing." << std::endl;
        // The unique_ptr is already in a safe state (empty), no need to manually handle.
    }

    std::cout << "End of main function. unique_ptr will go out of scope now." << std::endl;

    // 4. Observe the file being closed automatically when the unique_ptr goes out of scope.
    // The destructor of logFilePtr will be called here, which in turn
    // will invoke our 'fileDeleter' lambda, closing the file.

    return 0;
}

// why arent we using make unique here ?
// You've hit on a very important distinction! We are not using std::make_unique here because 
// std::make_unique is specifically designed for allocating memory using new (or new[] for arrays), 
// whereas fopen() acquires a resource in a fundamentally different way.

