#include <iostream>

void process_operation_2(int(int, int));
void process_operation_3(int()); // takes as parameter a function that returns int & takes no parameters

// explicit and canonical way to declare a function pointer parameter
void process_operation_0(int (*operation_func)(int, int)) {
    int result = operation_func(10, 5);
    std::cout << "Result of operation: " << result << std::endl;
}

// This declares a reference to a function.
// No nullptr: A reference must always refer to a valid function; it cannot be nullptr (like a function pointer can be).
// No re-seating: Once a reference is initialized to a function, it cannot be made to refer to a different function. (A function pointer can be reassigned to point to different functions.)
// Syntax: When calling the function through the reference, you use the direct function call syntax (operation_func(10, 5)), just as you would with the original function name. No dereferencing * is needed.
void process_operation_1(int (&operation_func)(int, int)) {
    int result = operation_func(10, 5);
    std::cout << "Result of operation: " << result << std::endl;
}

// This is the "shorthand" or "decaying" form for a function pointer parameter.
// How it works: In C++, when a function parameter is declared as a function type (e.g., ReturnType FunctionName(Args)), it is automatically adjusted by the compiler to be a pointer to that function type. This is similar to how an array parameter int arr[] decays to int* arr.
// Behind the scenes: The compiler treats int operation_func(int, int) exactly as if you had written int (*operation_func)(int, int). It's a syntactic convenience.
void process_operation_2(int operation_func(int, int)) {
    int result = operation_func(10, 5);
    std::cout << "Result of operation: " << result << std::endl;
}

int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int main() {
    process_operation_0(add);      // Pass a pointer to the 'add' function
    process_operation_0(subtract); // Pass a pointer to the 'subtract' function
    process_operation_1(add);      // Pass a reference to the  'add' function
    process_operation_1(subtract); // Pass a reference to the  'substract' function
    process_operation_2(add);      // Pass a pointer to the 'add' function
    process_operation_2(subtract); // Pass a pointer to the 'subtract' function

    // In process_operation_0(add), process_operation_0(subtract), process_operation_2(add), process_operation_2(subtract):
    // add decays to &add
    // Generally, it's considered better practice to just use the function name (add) rather than explicitly taking its address (&add).
    
    // For pointers to member functions (e.g., &MyClass::myMethod), the & is absolutely necessary 
    // because member functions behave differently and require the & to form a pointer-to-member type. 
    // However, for free functions (non-member functions) like add, it's not needed.
    return 0;
}
