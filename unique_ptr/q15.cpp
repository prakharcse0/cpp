// Conceptual: What is a "custom deleter" for std::unique_ptr? 
// Provide an example scenario where you might need one.

// A "custom deleter" for std::unique_ptr is a callable object (a function pointer, a lambda expression, or a function object/functor) that you provide to std::unique_ptr at its construction. Its purpose is to define how the managed resource should be deallocated or released when the std::unique_ptr goes out of scope.

// By default, std::unique_ptr<T> uses delete (for single objects) and std::unique_ptr<T[]> uses delete[] (for arrays) to deallocate memory. However, not all resources are acquired and released with new/delete. A custom deleter allows std::unique_ptr to manage virtually any resource that needs explicit release, providing a powerful way to extend RAII beyond just heap memory.

// How it works:

// When the std::unique_ptr goes out of scope, instead of simply calling delete ptr; (or delete[] ptr;), it calls the custom deleter function/object you provided, passing the raw pointer it manages to it.

// The custom deleter becomes part of the std::unique_ptr's type (e.g., std::unique_ptr<T, DeleterType>). This means:

// It's a compile-time mechanism.
// The size of the std::unique_ptr might increase if the deleter is a stateful lambda or a function object (though function pointers and stateless lambdas usually don't add size due to Empty Base Optimization).
// Example Scenario Where You Might Need One: Managing a C File Handle
// One of the most classic and clear examples is managing a C-style file handle obtained via fopen() which must be closed with fclose().

// Problem without a Custom Deleter:

// If you tried to manage a FILE* with a default std::unique_ptr:

// C++

// // THIS IS INCORRECT AND WILL CRASH!
// #include <iostream>
// #include <memory> // For std::unique_ptr
// #include <cstdio> // For FILE, fopen, fclose

// void do_file_stuff_incorrect() {
//     std::unique_ptr<FILE> file_ptr(fopen("example.txt", "w")); // Incorrect usage!
//     // file_ptr now "owns" a FILE*

//     if (file_ptr) {
//         fprintf(file_ptr.get(), "Hello world!\n");
//     }

//     // When file_ptr goes out of scope, its destructor will call `delete file_ptr.get()`.
//     // This is incorrect for a FILE*! `fopen` returns a pointer to a struct allocated by the C runtime,
//     // not by `new`. Calling `delete` on it is undefined behavior, likely a crash.
// } // CRASH HERE
// Solution with a Custom Deleter:

// You need a way to tell std::unique_ptr to call fclose() instead of delete. This is where a custom deleter comes in.

// C++

// #include <iostream>
// #include <memory> // For std::unique_ptr
// #include <cstdio> // For FILE, fopen, fclose

// // Option 1: Using a function pointer
// // This function will be called by unique_ptr to close the file
// void closeFile(FILE* file) {
//     if (file) { // Always check if file is not nullptr before closing
//         std::cout << "  Closing file with fclose()." << std::endl;
//         fclose(file);
//     }
// }

// // Option 2: Using a lambda expression (more common in modern C++)
// auto file_deleter_lambda = [](FILE* file) {
//     if (file) {
//         std::cout << "  Closing file with lambda deleter." << std::endl;
//         fclose(file);
//     }
// };

// void do_file_stuff_correct() {
//     std::cout << "Starting file operations with unique_ptr and custom deleter." << std::endl;

//     // Using Option 1: Function pointer as deleter
//     // The type signature for unique_ptr needs to include the deleter type
//     std::unique_ptr<FILE, decltype(&closeFile)> file_ptr1(fopen("output1.txt", "w"), &closeFile);

//     if (file_ptr1) {
//         fprintf(file_ptr1.get(), "This is written to output1.txt\n");
//     } else {
//         std::cerr << "Error: Could not open output1.txt" << std::endl;
//     }
//     std::cout << "  file_ptr1 going out of scope." << std::endl;
//     // When file_ptr1 goes out of scope, closeFile() is automatically called on its managed FILE*.

//     std::cout << "\nStarting second file operations with lambda deleter." << std::endl;

//     // Using Option 2: Lambda expression as deleter
//     // For stateless lambdas, the type can be deduced or specified with decltype
//     std::unique_ptr<FILE, decltype(file_deleter_lambda)> file_ptr2(fopen("output2.txt", "w"), file_deleter_lambda);

//     if (file_ptr2) {
//         fprintf(file_ptr2.get(), "This is written to output2.txt\n");
//     } else {
//         std::cerr << "Error: Could not open output2.txt" << std::endl;
//     }
//     std::cout << "  file_ptr2 going out of scope." << std::endl;
//     // When file_ptr2 goes out of scope, the lambda is automatically called on its managed FILE*.

//     std::cout << "Finished file operations." << std::endl;
// }

// int main() {
//     do_file_stuff_correct();
//     return 0;
// }
// Output:

// Starting file operations with unique_ptr and custom deleter.
//   file_ptr1 going out of scope.
//   Closing file with fclose().

// Starting second file operations with lambda deleter.
//   file_ptr2 going out of scope.
//   Closing file with lambda deleter.
// Finished file operations.
// Other Scenarios Where Custom Deleters are Useful:

// Memory from malloc/calloc: If you allocate memory using C-style malloc or calloc, you must free it with free().
// C++

// std::unique_ptr<char, decltype(&free)> buffer((char*)malloc(100), &free);
// Operating System Handles: Managing HANDLEs on Windows (which need CloseHandle) or file descriptors on Unix-like systems (which need close).
// Resource Pools: Releasing a resource back to a pool instead of destroying it (e.g., returning a database connection to a connection pool).
// Specialized Resource Managers: Any scenario where a resource needs specific cleanup logic that isn't delete or delete[].
// Custom deleters are a powerful feature of std::unique_ptr that allow it to manage virtually any type of resource using the robust and exception-safe RAII principle.

