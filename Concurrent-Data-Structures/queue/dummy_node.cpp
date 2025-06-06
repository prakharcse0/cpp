#include <memory>
#include <memory>
#include <iostream>

// Queue with dummy node to separate head and tail operations
// This solves the problem where head==tail when queue has one element
// Key insight: Always maintain at least one node (dummy) to separate head/tail access
template<typename T>
class queue
{
private:
    struct node
    {
        std::shared_ptr<T> data;    // Data stored as pointer (dummy node has nullptr)
        std::unique_ptr<node> next;
    };
    
    std::unique_ptr<node> head;  // Points to dummy node when empty
    node* tail;                  // Points to last real node (or dummy when empty)

public:
    // Constructor creates initial dummy node
    // Both head and tail point to same dummy node when empty
    queue() : head(new node), tail(head.get()) {}
    
    queue(const queue& other) = delete;
    queue& operator=(const queue& other) = delete;
    
    std::shared_ptr<T> try_pop()
    {
        // Empty check: if head==tail, only dummy node exists
        if (head.get() == tail)
        {
            return std::shared_ptr<T>();
        }
        
        // Get data from head node (which is no longer dummy after first push)
        std::shared_ptr<T> const res(head->data);
        
        // Move head to next node, old head becomes new dummy
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        
        return res;
    }
    
    void push(T new_value)
    {
        // Create data on heap
        std::shared_ptr<T> new_data(
            std::make_shared<T>(std::move(new_value)));
        
        // Create new dummy node
        std::unique_ptr<node> p(new node);
        
        // Current tail (dummy) gets the data - becomes real node
        tail->data = new_data;
        
        // Link current tail to new dummy
        node* const new_tail = p.get();
        tail->next = std::move(p);
        tail = new_tail;  // Update tail to point to new dummy
    }
    
    bool empty() const
    {
        return head.get() == tail;
    }
};

int main()
{
    queue<int> q;
    
    std::cout << "=== Dummy Node Queue Demo ===" << std::endl;
    std::cout << "Queue empty: " << q.empty() << std::endl;
    
    // Demonstrate that dummy node separates head/tail operations
    q.push(10);
    std::cout << "Pushed: 10 (head != tail now)" << std::endl;
    std::cout << "Queue empty: " << q.empty() << std::endl;
    
    q.push(20);
    q.push(30);
    std::cout << "Pushed: 20, 30" << std::endl;
    
    // Pop all values
    while (auto value = q.try_pop())
    {
        std::cout << "Popped: " << *value << std::endl;
    }
    
    std::cout << "Queue empty: " << q.empty() << std::endl;
    std::cout << "Note: Dummy node still exists, head==tail again" << std::endl;
    
    return 0;
}