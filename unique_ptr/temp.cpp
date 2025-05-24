#include <iostream>
#include <string>
#include <memory>
#include <vector>

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


void processBook(std::unique_ptr<Book> &&book) {
    book -> read();
    // std::unique_ptr<Book> newBook = std::move(book);
}

void processVector(std::vector<int> &&v) {
    // std::vector<int> vi = std::move(v);
}

int main() {
    std::vector<int> v = {1, 2, 3, 4};
    processVector(std::move(v));
    std::cout <<v.size() <<std::endl;

    std::unique_ptr<Book> anotherBook = std::make_unique<Book>("Clean Code");
    processBook(std::move(anotherBook));
    if(anotherBook)
        std::cout <<"Another book still exits" <<std::endl;
    else
        std::cout <<"Another book is empty" <<std::endl;
    anotherBook->read();

    return 0;
}

// Output:
// rust@victus:~/cse_1/cpp/unique_ptr$ ./a.out
// 4
// Book created: Clean Code
// Reading Clean Code
// Another book still exits
// Reading Clean Code
// Book destroyed: Clean Code

// On uncommenting     // std::unique_ptr<Book> newBook = std::move(book); &&     // std::vector<int> vi = std::move(v);
// rust@victus:~/cse_1/cpp/unique_ptr$ ./a.out
// 0
// Book created: Clean Code
// Reading Clean Code
// Book destroyed: Clean Code
// Another book is empty
// Segmentation fault (core dumped)