// Challenge 3: Circular Dependencies & std::weak_ptr
// Task:
// You are designing a social network application where User objects can "follow" each other.

// 1. Define two classes: UserA and UserB.

// 2. In UserA, include a std::shared_ptr<UserB> member named follows_B.

// 3. In UserB, initially include a std::shared_ptr<UserA> member named follows_A.

// 4. Implement constructors and destructors for both classes that print messages (e.g., "UserA created", "UserA destroyed").

// 5. In main, create std::shared_ptr instances for UserA and UserB.

// 6. Establish a circular dependency: userA->follows_B = userB; and userB->follows_A = userA;.

// 7. Observe the output when the shared_ptrs go out of scope. Explain why the destructors are not called, leading to a memory leak.

// 8. Modify UserB's follows_A member from std::shared_ptr<UserA> to std::weak_ptr<UserA>.

// 9. Rerun the main code. Observe the output again. Explain how std::weak_ptr resolves the memory leak.

// 10. Bonus: Inside UserB, add a method check_followed_A() that attempts to lock follows_A and prints whether UserA is still alive. 
// Call this method from main after establishing the connection, and then again after userA (the external shared_ptr to UserA) has gone out of scope (e.g., in a separate inner scope).

// Concepts Tested: Circular dependencies, memory leaks, std::weak_ptr as a solution, lock(), expired(), 
// suse_count() for weak_ptr (though not directly use_count() on the weak_ptr itself, but on the shared_ptr it observes), observer pattern (conceptual).

#include <iostream>
#include <memory>

class UserA {
public:
    std::shared_ptr<UserB> follows_b;
    UserA() {
        std::cout <<"User A created" <<std::endl;
    }
    ~UserA() {
        std::cout <<"User A destroyed" <<std::endl;
    }


};

class UserB {
public:
    std::shared_ptr<UserA> follows_a;
    UserB() {
        std::cout <<"User B created" <<std::endl;
    }
    ~UserB() {
        std::cout <<"User B destroyed" <<std::endl;
    }

};