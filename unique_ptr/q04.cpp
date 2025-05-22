// Coding: Consider a simple class Book with a constructor that prints "Book created: [title]" and a destructor that prints "Book destroyed: [title]".

#include <iostream>
#include <string>

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

// Write a main function where you:
// Create a std::unique_ptr named myBook managing a Book object with the title "The C++ Primer".
// Call the read() method on the Book object using myBook.
// Observe the output when myBook goes out of scope.

#include <memory>

int main() {
    std::unique_ptr<Book> myBook = std::make_unique<Book>("The C++ Primer"); //why is it compiling ? doesnt the constructor asks for string & - a lvalue referenc ?

    // When you write "The C++ Primer", this is a C-style string literal. In C++, string literals are const char[].
    // The compiler performs an implicit conversion:

    // The const char[] literal "The C++ Primer" is used to create a temporary std::string object. This temporary std::string is an rvalue.
    // A const lvalue reference (const std::string&) can bind to an rvalue (a temporary object). This is a very important rule in C++. 
    // When it binds to a temporary, it effectively extends the lifetime of that temporary for the duration of the function call (the constructor, in this case).

    // If your constructor were Book(std::string& t) (a non-const lvalue reference), 
    // then it would not compile because a non-const lvalue reference cannot bind to an rvalue (a temporary). 
    // This rule prevents you from accidentally modifying a temporary object that's about to disappear.

    // So, in summary: Yes, const std::string& can bind to temporary std::string objects (rvalues), allowing your code to compile.

    myBook->read();
}

// Output:
// Book created: The C++ Primer
// Reading The C++ Primer
// Book destroyed: The C++ Primer

