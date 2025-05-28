// You can pass a member function pointer as the function, 
// provided you supply a suitable object pointer as the first argument:

#include <thread>


class X
{
public:
    void do_lengthy_work() {

    }

    static void static_do_work() {
        // std::cout << "X::static_do_work() is running." << std::endl;
    }
};

// undefined reference to symbol_name": This is the hallmark signature of a linker error. 
// It means the compiler saw a declaration for symbol_name (e.g., a function, a global variable) and generated code that calls or uses it, 
// but the linker cannot find the actual implementation or definition of that symbol in any of the object files or libraries it's trying to combine.


class big_object {
public:

    int data;

    void prepare_data(int data_) {
        data = data_;
    }
};

void process_big_object(std::unique_ptr<big_object>) {

}


int main() {
    X my_x;
    std::thread t(&X::do_lengthy_work, &my_x);
    // This code will invoke my_x.do_lengthy_work() on the new thread, 
    // because the address of my_x is supplied as the object pointer B. 
    // You can also supply arguments to such a member function call: 
    // the third argument to the std::thread constructor will
    // be the first argument to the member function and so forth.

    // // The & before X::do_lengthy_work is necessary 
    // because X::do_lengthy_work (a non-static member function) is not just a regular function; 
    // it's a pointer-to-member-function.

    std::thread t_static(&X::static_do_work);
    // Since X::static_do_work doesn't operate on an instance of X, you only pass the function pointer

    // In C++, the name of a function, when used in an expression (like passing to std::thread's constructor), 
    // implicitly decays into a pointer to that function. 
    // It's similar to how an array name decays into a pointer to its first element.
    // So, global_function in that context is already treated as &global_function. 
    // You can explicitly write &global_function, and it compiles just fine, but it's redundant.


    std::unique_ptr<big_object> p(new big_object);
    p->prepare_data(42);
    std::thread t1(process_big_object,std::move(p));
    // By specifying std::move(p) in the std::thread constructor, the ownership of the
    // big_object is transferred first into internal storage for the newly created thread and
    // then into process_big_object

    t.join();
    t_static.join();
    t1.join();
}



// Another interesting scenario for supplying arguments is where the arguments
// can’t be copied but can only be moved: 
// the data held within one object is transferred over to another, leaving the original object “empty.” 

// An example of such a type is std::unique_ptr, 
// which provides automatic memory management for dynamically allocated objects. 

// Only one std::unique_ptr instance can point to a given object at a time, 
// and when that instance is destroyed, the pointed-to object is deleted. 
// The move constructor and move assignment operator allow the ownership of an object to be 
// transferred around between std::unique_ptr instances. 
// Such a transfer leaves the source object with a NULL pointer. 
// This moving of values allows objects of this type to be accepted as function
// parameters or returned from functions. 
// Where the source object is a temporary, the move is automatic, 
// but where the source is a named value, the transfer must be requested directly by invoking std::move(). 
// The following example shows the use of std::move to transfer ownership of a dynamic object into a thread: