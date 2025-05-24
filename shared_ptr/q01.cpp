// Challenge 1: Basic Ownership & Creation

// Task:
// You have a Resource class.

// 1. Implement the Resource class with a constructor that prints "Resource constructed!" and a destructor that prints "Resource destroyed!". 
// Include a public method void use() that prints "Resource used!".

// 2. In main, create a std::shared_ptr to a Resource object using std::make_shared. Call its use() method.

// 3. Create another std::shared_ptr and make it share ownership with the first one.

// 4. Print the use_count() of both shared_ptrs after the copy.

// 5. Introduce a new scope ({}) and inside it, create a third shared_ptr that also shares ownership. Print the use_count() inside this scope.

// 6. Observe and explain the output, specifically noting when the Resource object is constructed and destroyed, and how the use_count changes.

// Concepts Tested: std::shared_ptr basics, std::make_shared, shared ownership, use_count(), object lifetime, RAII.

#include <iostream>
#include <memory>

class Resource {

public:
    Resource() {
        std::cout <<"Resource constructed" <<std::endl;
    }

    ~Resource() {
        std::cout <<"Resource destroyed" <<std::endl;
    }

    void use() {
        std::cout <<"Resource used!" <<std::endl;
    }

};

int main() {
    std::shared_ptr<Resource> ptr1 = std::make_shared<Resource>();
    std::shared_ptr<Resource> ptr2 = ptr1;
    std::cout <<"ptr1.use_count(): " <<ptr1.use_count() <<std::endl;
    std::cout <<"ptr2.use_count(): " <<ptr2.use_count() <<std::endl;
    {
        std::shared_ptr<Resource> ptr3 = ptr2;
        std::cout <<"ptr1.use_count(): " <<ptr2.use_count() <<std::endl;
    }
    std::cout <<"ptr1.use_count(): " <<ptr1.use_count() <<std::endl;

    return 0;
}
