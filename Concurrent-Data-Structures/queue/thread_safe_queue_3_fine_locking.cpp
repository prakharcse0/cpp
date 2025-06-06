#include <memory>
#include <mutex>
#include <iostream>
#include <thread>
#include <vector>

// Thread-safe queue using fine-grained locking (separate mutexes for head and tail)
// Key insight: Dummy node ensures push() and try_pop() never operate on same node
template<typename T>
class threadsafe_queue
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    
    std::mutex head_mutex;           // Protects head pointer
    std::unique_ptr<node> head;
    std::mutex tail_mutex;           // Protects tail pointer  
    node* tail;
    
    // Helper: safely get tail pointer under lock
    node* get_tail()
    {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }
    
    // Helper: remove head node under lock
    // Returns nullptr if queue is empty
    std::unique_ptr<node> pop_head()
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        
        // Check if empty (head==tail means only dummy exists)
        if (head.get() == get_tail())
        {
            return nullptr;
        }
        
        // Remove head node
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

public:
    threadsafe_queue() : head(new node), tail(head.get()) {}
    
    threadsafe_queue(const threadsafe_queue& other) = delete;
    threadsafe_queue& operator=(const threadsafe_queue& other) = delete;
    
    // Thread-safe pop operation
    std::shared_ptr<T> try_pop()
    {
        std::unique_ptr<node> old_head = pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }
    
    // Thread-safe push operation
    void push(T new_value)
    {
        // Allocate outside of lock for better concurrency
        std::shared_ptr<T> new_data(
            std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);
        
        node* const new_tail = p.get();
        
        // Only lock when modifying the queue structure
        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex);
            tail->data = new_data;        // Dummy becomes real node
            tail->next = std::move(p);    // Link to new dummy
            tail = new_tail;              // Update tail
        }
        // Lock released - other threads can proceed
    }
};

// Test concurrent operations
void producer(threadsafe_queue<int>& q, int start, int count)
{
    for (int i = 0; i < count; ++i)
    {
        q.push(start + i);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void consumer(threadsafe_queue<int>& q, int& consumed_count)
{
    while (consumed_count < 20) // Stop after consuming 20 items
    {
        if (auto value = q.try_pop())
        {
            std::cout << "Consumed: " << *value << std::endl;
            ++consumed_count;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

int main()
{
    threadsafe_queue<int> q;
    int consumed = 0;
    
    std::cout << "=== Fine-Grained Locking Demo ===" << std::endl;
    std::cout << "Starting concurrent producers and consumer..." << std::endl;
    
    // Create multiple producer threads
    std::vector<std::thread> producers;
    producers.emplace_back(producer, std::ref(q), 0, 10);   // Producer 1: 0-9
    producers.emplace_back(producer, std::ref(q), 100, 10); // Producer 2: 100-109
    
    // Create consumer thread
    std::thread consumer_thread(consumer, std::ref(q), std::ref(consumed));
    
    // Wait for all threads
    for (auto& t : producers)
        t.join();
    consumer_thread.join();
    
    std::cout << "All threads completed. Fine-grained locking allows:" << std::endl;
    std::cout << "- Multiple producers can allocate concurrently" << std::endl;
    std::cout << "- Push and pop can proceed concurrently" << std::endl;
    std::cout << "- Only brief locks during pointer updates" << std::endl;
    
    return 0;
}