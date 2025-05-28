// It’s also possible to get the reverse scenario: the object is copied, and what you wanted was a reference. 
// This might happen if the thread is updating a data structure that’s passed in by reference,

// Use std::ref() for that


#include <iostream>
#include <thread>
#include <string>   // For std::string
#include <functional> // For std::ref 

using widget_id = unsigned int;

// --- Basic widget_data Class ---
class widget_data {
public:
    int value; // A simple data member to observe changes

    widget_data() : value(0) {
        std::cout << "[widget_data] Constructed (value: " << value << ")" << std::endl;
    }

    // Explicitly showing copy constructor to see when copies happen
    widget_data(const widget_data& other) : value(other.value) {
        std::cout << "[widget_data] COPY CONSTRUCTED (value: " << value << ")" << std::endl;
    }

    // ~widget_data() { std::cout << "[widget_data] Destructed (value: " << value << ")" << std::endl; }
};

// --- Function to be run by the thread ---
// This function *expects* to modify 'data' by reference.
void update_data_for_widget(widget_id w, widget_data& data) {
    std::cout << "[Thread " << w << "] Attempting to update data. Original value: " << data.value << std::endl;
    data.value = 100; // Modify the data
    std::cout << "[Thread " << w << "] Data updated. New value: " << data.value << std::endl;
}


// --- The problematic function from your example ---
void oops_again(widget_id w) {
    widget_data data; // (A) Local object
    std::cout << "[Main] Data 'data' created. Value: " << data.value << std::endl;

    std::thread t(update_data_for_widget, w, data);
    // PROBLEM: 'data' (a widget_data object) is COPIED into the thread's internal storage.
    // 'update_data_for_widget' receives a reference to this internal COPY, not the original 'data'.
    // Changes made by the thread will NOT affect the 'data' object in 'oops_again()'.

    t.join(); // Wait for the thread to finish

    std::cout << "[Main] After thread finishes, original 'data' value: " << data.value << std::endl;
}

// --- The corrected function (showing the solution) ---
void not_oops_again(widget_id w) {
    widget_data data;
    std::cout << "[Main] Data 'data' created. Value: " << data.value << std::endl;

    // SOLUTION: Use std::ref() to explicitly pass 'data' by reference to the thread.
    std::thread t(update_data_for_widget, w, std::ref(data));

    t.join();

    std::cout << "[Main] After thread finishes, original 'data' value: " << data.value << std::endl;
}


int main() {
    std::cout << "--- Demonstrating OOPS_AGAIN (Problematic Behavior) ---" << std::endl;
    oops_again(1);

    std::cout << "\n--- Demonstrating NOT_OOPS_AGAIN (Correct Behavior) ---" << std::endl;
    not_oops_again(2);

    return 0;
}