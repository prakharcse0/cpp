// Coding: Using the Book class:
// Create std::unique_ptr<Book> manualBook for "The Mythical Man-Month".
// Obtain the raw pointer using release() and store it in Book* rawManualBook.
// Verify that manualBook is empty.
// Call read() on rawManualBook.
// Manually delete rawManualBook and set it to nullptr.

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
    std::unique_ptr<Book> manualBook = std::make_unique<Book>("The Mythical Man-Month");
    Book *rawManualBook = manualBook.release();
    if(manualBook)
        std::cout <<"Manual book still exits" <<std::endl;
    else
        std::cout <<"Manual book is empty" <<std::endl;
    rawManualBook->read();
    
    return 0;
}
