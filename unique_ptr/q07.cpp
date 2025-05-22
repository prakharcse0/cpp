// Coding: Modify the Book example from Section 1. In main:

// Create std::unique_ptr<Book> book1 for "Effective C++".
// Create std::unique_ptr<Book> book2 and transfer ownership of "Effective C++" from book1 to book2.
// Verify that book1 is empty after the transfer (print a message).
// Use book2 to call read().

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


int main() {
    std::unique_ptr<Book> book1 = std::make_unique<Book>("Effective C++");
    std::unique_ptr<Book> book2 = std::move(book1);
    if(!book1)
        std::cout <<"book1 is empty" <<std::endl;
    return 0;
}

// Output:
// Book created: Effective C++
// book1 is empty
// Book destroyed: Effective C++