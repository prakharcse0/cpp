// Coding:
// Write code to create a std::unique_ptr that manages a dynamically allocated array of 3 ints.
// Initialize the elements of the array (e.g., 0, 10, 20).
// Print the elements using array-style access (p[i]).

#include <iostream>
#include <memory>

int main() {
    std::unique_ptr<int[]> arr = std::make_unique<int[]>(3);
    arr[0] = 0, arr[1] = 10, arr[2] = 20;
    for(int i = 0; i < 3; ++i) {
        std::cout <<arr[i] <<' ';
    }

    std::cout <<std::endl;
}