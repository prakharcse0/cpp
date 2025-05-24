Challenge 4: shared_ptr vs. unique_ptr & Design Choice
Task:
You need to manage dynamically allocated Data objects.

Implement a Data class with a constructor/destructor printing messages. It should have a public int value member.
Scenario A (Exclusive Ownership):
Implement a function create_unique_data() that returns a std::unique_ptr<Data>.
In main, call create_unique_data() and demonstrate ownership transfer by moving the unique_ptr to another unique_ptr. Access and modify the value through both.
Explain why std::unique_ptr is suitable here and why it's more efficient than std::shared_ptr for exclusive ownership.
Scenario B (Shared Ownership):
Implement a function create_shared_data() that returns a std::shared_ptr<Data>.
In main, call create_shared_data(). Create multiple copies of this shared_ptr and pass them to different functions (e.g., process_data_1(std::shared_ptr<Data>&), process_data_2(std::shared_ptr<Data>)).
Inside these functions, print the use_count().
Explain why std::shared_ptr is necessary here and how it handles the lifetime when passed around.
Discussion:
When deciding between std::unique_ptr and std::shared_ptr, what is the primary decision factor?
Provide a brief example where using a raw pointer would be acceptable/necessary, and explain why a smart pointer might be overkill or inappropriate in that specific context (e.g., function parameter that only observes, not owns).
Concepts Tested: std::unique_ptr vs. std::shared_ptr comparison, ownership semantics, efficiency considerations, passing smart pointers as arguments, raw pointer usage scenarios.

