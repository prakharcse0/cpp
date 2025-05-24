Challenge 5: shared_ptr and Polymorphism (Advanced)
Task:
You have a base class Shape and derived classes Circle and Rectangle. You want to manage these polymorphic objects using shared_ptr.

Define a base class Shape with a virtual destructor (important!) and a pure virtual method void draw(). Implement a constructor and destructor that print messages.
Define two derived classes Circle and Rectangle that inherit from Shape. Implement their constructors, destructors, and override the draw() method to print "Drawing Circle" or "Drawing Rectangle", respectively.
In main, create a std::shared_ptr<Shape> that points to a Circle object. Call its draw() method.
Create another std::shared_ptr<Shape> that points to a Rectangle object. Call its draw() method.
Pass the shared_ptr pointing to the Circle to a function process_shape(std::shared_ptr<Shape> s). Inside process_shape, call s->draw() and print s.use_count().
Critical Question: Explain why the Shape destructor must be virtual when dealing with polymorphic objects managed by shared_ptr (or any smart pointer/raw pointer in a polymorphic context). What happens if it's not virtual?
Concepts Tested: Polymorphism with smart pointers, virtual destructors, object slicing (conceptual), proper inheritance for smart pointer management.

