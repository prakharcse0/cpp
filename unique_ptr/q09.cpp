// Coding: Consider two functions:

// void processBook(std::unique_ptr<Book> book): This function should take ownership of the Book. Inside, it calls read() and then the Book should be destroyed when book goes out of scope.
// void inspectBook(const Book& book): This function should only observe the Book without taking ownership. It calls read().
// In main, create a std::unique_ptr<Book> anotherBook for "Clean Code".

// Demonstrate calling inspectBook using anotherBook.
// Then, demonstrate calling processBook using anotherBook.
// Verify that anotherBook is empty after the call to processBook.

#include <iostream>
#include <string>
#include <memory>

class Book {
public:
    std::string title;
    Book(const std::string& t) : title(t) {
        std::cout << "Book created: " << title << std::endl;
    }
    ~Book() {
        std::cout << "Book destroyed: " << title << std::endl;
    }
    void read() const {
        std::cout << "Reading " << title << std::endl;
    }
};

// void processBook(std::unique_ptr<Book> &&book) // wrong, && doesnt take ownership, just rvalue reference
void processBook(std::unique_ptr<Book> book) {
    book -> read();
}

void inspectBook(std::unique_ptr<Book> &book) {
    book -> read();
}

int main() {
    std::unique_ptr<Book> anotherBook = std::make_unique<Book>("Clean Code");
    inspectBook(anotherBook);
    processBook(std::move(std::make_unique<Book>("New Book")));
    if(anotherBook)
        std::cout <<"Another book still exits" <<std::endl;
    else
        std::cout <<"Another book is empty" <<std::endl;
    anotherBook->read();
    return 0;
}

// explain && in context of unique_ptr & move


// Let's zero in on && specifically in the context of std::unique_ptr and move semantics. This is a very important and common intersection in modern C++.

// && in the Context of std::unique_ptr and Move Semantics
// std::unique_ptr is designed to provide exclusive ownership of a dynamically allocated resource. This means it cannot be copied. If it could be copied, you'd have two unique_ptrs pointing to the same memory, leading to a double-free when they both go out of scope.

// However, sometimes you need to transfer this exclusive ownership from one std::unique_ptr to another. This is where && (rvalue references) and move semantics come into play.

// Here's how && is crucial for std::unique_ptr's move behavior:

// Enabling std::unique_ptr's Move Constructor and Move Assignment Operator:

// std::unique_ptr explicitly deletes its copy constructor and copy assignment operator.
// It provides a move constructor (std::unique_ptr(std::unique_ptr&& other) noexcept) and a move assignment operator (std::unique_ptr& operator=(std::unique_ptr&& other) noexcept).
// The && in these signatures is what allows them to accept an rvalue unique_ptr. An rvalue unique_ptr is either a temporary unique_ptr (like std::make_unique<T>()'s return value) or a unique_ptr explicitly cast to an rvalue using std::move().
// What happens in the move:
// When a unique_ptr is moved, it's not a deep copy. Instead, the move constructor/assignment operator performs these steps:

// The destination unique_ptr takes the raw pointer from the source unique_ptr.
// The source unique_ptr's internal raw pointer is set to nullptr.
// This efficiently transfers ownership: the destination now manages the resource, and the source no longer owns it, preventing double-free.
// std::move() as the Bridge to &&:

// You often want to move from an lvalue unique_ptr (a named unique_ptr variable).
// std::move() is a function that simply casts its argument to an rvalue reference (T&&). It doesn't perform any actual data movement itself.
// This rvalue reference (the result of std::move()) then allows the compiler to select the unique_ptr's move constructor or move assignment operator.
// Example:

// C++

// std::unique_ptr<int> p1 = std::make_unique<int>(10); // p1 owns the int(10)
// // p1 is an lvalue (it has a name)

// std::unique_ptr<int> p2 = std::move(p1); // 'std::move(p1)' evaluates to std::unique_ptr<int>&& (an rvalue reference)
//                                       // This calls std::unique_ptr's MOVE CONSTRUCTOR:
//                                       // p2(std::unique_ptr<int>&& other)
//                                       // p2 now owns the int(10)
//                                       // p1 is set to nullptr
// std::unique_ptr as a Function Parameter Type:
// You've experienced this directly. && is critical here.

// void f(std::unique_ptr<T> book) (Pass by Value - Takes Ownership):

// This is the idiomatic way for a function to take ownership of a std::unique_ptr.
// Because std::unique_ptr is non-copyable, if you pass an lvalue unique_ptr to this function, you must use std::move().
// The parameter book inside f is move-constructed from the incoming unique_ptr.
// When book goes out of scope, the resource it owns is destroyed.
// <!-- end list -->

// C++

// void processBook(std::unique_ptr<Book> book) { // 'book' is the new owner
//     book->read();
// } // Book destroyed when 'book' goes out of scope

// // Usage:
// std::unique_ptr<Book> myBook = std::make_unique<Book>("My Title");
// processBook(std::move(myBook)); // 'myBook' becomes empty after this call
// void f(std::unique_ptr<T> &&book) (Pass by Rvalue Reference - Enables Move or Temporary Use):

// This parameter type ensures that only rvalue unique_ptrs can be passed.
// As discussed, the book parameter itself is an lvalue within the function.
// If the function wants to take ownership (e.g., store it in a member variable), it must explicitly std::move(book) inside.
// If it doesn't std::move(book) internally, the original unique_ptr (if it was a temporary, or the result of std::move()) will be destroyed when its lifetime ends (which is typically at the end of the full expression or function call). This is what happened in your last example.
// <!-- end list -->

// C++

// void storeBook(std::unique_ptr<Book> &&book_to_store) {
//     // MyClass member: std::unique_ptr<Book> owned_book_;
//     // owned_book_ = std::move(book_to_store); // Explicitly move to take ownership
// }

// void temporaryUse(std::unique_ptr<Book> &&book_for_read) {
//     book_for_read->read(); // Use the resource, but don't take ownership (don't move it internally)
// } // The original temporary unique_ptr will be destroyed here, taking Book with it.

// // Usage:
// storeBook(std::make_unique<Book>("Stored Book")); // Move to 'book_to_store', then to 'owned_book_'
// temporaryUse(std::make_unique<Book>("Temp Read Book")); // Used then destroyed by parameter lifetime extension
// In a nutshell:

// && is the language feature that allows std::unique_ptr to exist as a non-copyable, movable smart pointer. It provides the mechanism for std::unique_ptr's move constructor and move assignment operator, enabling the safe and efficient transfer of exclusive resource ownership without resorting to deep copies or manual pointer management. When you see std::move() used with unique_ptr, you're leveraging && to signal an ownership transfer.