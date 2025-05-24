#include <iostream>
#include <memory>


void processUniquePtr(std::unique_ptr<int>&& ptr) {
    auto mine = std::move(ptr); // Takes ownership
    std::cout << "Took ownership of unique_ptr\n";
}

void processUniquePtrRef(std::unique_ptr<int>& ptr) {
    auto mine = std::move(ptr); // Takes ownership
    std::cout << "Took ownership of unique_ptr\n";
}

int main() {
    std::unique_ptr<int> i = std::make_unique<int>(4);
    processUniquePtr(std::move(i));
    if(i)
        std::cout <<"i is not empty" <<std::endl;
    else
        std::cout <<"i is empty" <<std::endl;

    i = std::make_unique<int>(5);
    processUniquePtrRef(i);
    if(i)
        std::cout <<"i is not empty" <<std::endl;
    else
        std::cout <<"i is empty" <<std::endl;
    
    return 0; 
}