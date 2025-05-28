// In C++, assert is a powerful debugging macro used to test assumptions made by the programmer. 
// It's designed to catch logical errors and conditions that should never occur if the program is working correctly.

// Purpose: To verify a condition that is expected to be true at a specific point in the program's execution.

// Header: Defined in the <cassert> (C++ style) or <assert.h> (C style) header.

// Syntax: assert(expression);
// expression must be a scalar type that can be contextually converted to bool.

// Behavior (Debug Builds):
// If expression evaluates to true (non-zero), assert does nothing, and the program continues execution normally.

// If expression evaluates to false (zero), assert performs the following actions:
// Prints a diagnostic message to the standard error stream (usually including the failed expression, filename, line number, and function name).
// Calls std::abort(), which causes the program to terminate abnormally and immediately.

#include <iostream>
#include <cassert> // Required for assert

double divide(double numerator, double denominator) {
    // Assert that the denominator is not zero.
    // This is a programmer's assumption: caller should not pass 0.
    // If it happens, it's a bug that should be caught during development.
    assert(denominator != 0.0 && "Denominator cannot be zero!"); // C++11 onwards, can often add a message

    return numerator / denominator;
}

int main() {
    std::cout << "Dividing 10 by 2: " << divide(10, 2) << std::endl;

    // This call will trigger the assert if NDEBUG is not defined
    std::cout << "Attempting to divide by zero..." << std::endl;
    divide(10, 0); // This will cause the program to terminate if asserts are enabled

    std::cout << "This line will not be reached if assert fails." << std::endl;

    return 0;
}