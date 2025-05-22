// Coding: Write a factory function createBook(const std::string& title) that returns a std::unique_ptr<Book> managing a new Book object. In main:
// Call createBook("Design Patterns") and store the result in std::unique_ptr<Book> myFactoryBook.
// Call myFactoryBook->read().
// Observe the destruction.

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

std::unique_ptr<Book> createBook(const std::string &title) {
    return std::make_unique<Book>(title);
}

int main() {
    std::unique_ptr<Book> myFactoryBook = createBook("Design Pattern");
    myFactoryBook->read();

    return 0;
}