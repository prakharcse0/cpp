#include <memory>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <thread>
#include <vector>

// Complete thread-safe queue with waiting capabilities
// Adds condition variables for blocking operations
template<typename T>
class threadsafe_queue
{
private:
    struct node
    {
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;
    };
    
    std::mutex head_mutex;
    std::unique_ptr<node> head;
    std::mutex tail_mutex;
    node* tail;
    std::condition_variable data_cond;  // Signals when data is available

    node* get_tail()
    {
        std::lock_guard<std::mutex> tail_lock(tail_mutex);
        return tail;
    }
    
    // Non-blocking pop helper
    std::unique_ptr<node> try_pop_head()
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail())
        {
            return std::unique_ptr<node>();
        }
        return pop_head();
    }
    
    // Non-blocking pop with value extraction helper
    std::unique_ptr<node> try_pop_head(T& value)
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        if (head.get() == get_tail())
        {
            return std::unique_ptr<node>();
        }
        value = std::move(*head->data);  // Extract value before removing node
        return pop_head();
    }
    
    // Blocking wait for data
    std::unique_lock<std::mutex> wait_for_data()
    {
        std::unique_lock<std::mutex> head_lock(head_mutex);
        // Wait until queue is not empty (head != tail)
        data_cond.wait(head_lock, [&] { return head.get() != get_tail(); });
        return std::move(head_lock);  // Return lock to caller
    }
    
    // Blocking pop helper
    std::unique_ptr<node> wait_pop_head()
    {
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        return pop_head();
    }
    
    // Blocking pop with value extraction helper
    std::unique_ptr<node> wait_pop_head(T& value)
    {
        std::unique_lock<std::mutex> head_lock(wait_for_data());
        value = std::move(*head->data);
        return pop_head();
    }
    
    std::unique_ptr<node> pop_head()
    {
        std::unique_ptr<node> old_head = std::move(head);
        head = std::move(old_head->next);
        return old_head;
    }

public:
    threadsafe_queue() : head(new node), tail(head.get()) {}
    
    threadsafe_queue(const threadsafe_queue& other) = delete;
    threadsafe_queue& operator=(const threadsafe_queue& other) = delete;
    
    // Non-blocking operations (return immediately)
    std::shared_ptr<T> try_pop()
    {
        std::unique_ptr<node> old_head = try_pop_head();
        return old_head ? old_head->data : std::shared_ptr<T>();
    }
    
    bool try_pop(T& value)
    {
        std::unique_ptr<node> const old_head = try_pop_head(value);
        return old_head != nullptr;
    }
    
    // Blocking operations (wait until data available)
    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_ptr<node> const old_head = wait_pop_head();
        return old_head->data;
    }
    
    void wait_and_pop(T& value)
    {
        std::unique_ptr<node> const old_head = wait_pop_head(value);
    }
    
    void push(T new_value)
    {
        std::shared_ptr<T> new_data(
            std::make_shared<T>(std::move(new_value)));
        std::unique_ptr<node> p(new node);
        
        {
            std::lock_guard<std::mutex> tail_lock(tail_mutex);
            tail->data = new_data;
            node* const new_tail = p.get();
            tail->next = std::move(p);
            tail = new_tail;
        }
        // Notify waiting threads that data is available
        data_cond.notify_one();
    }
    
    bool empty()
    {
        std::lock_guard<std::mutex> head_lock(head_mutex);
        return (head.get() == get_tail());
    }
};

// Demonstrate waiting behavior
void slow_producer(threadsafe_queue<int>& q)
{
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "Producer: Adding item after 2 second delay..." << std::endl;
    q.push(42);
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Producer: Adding second item..." << std::endl;
    q.push(100);
}

void waiting_consumer(threadsafe_queue<int>& q, int id)
{
    std::cout << "Consumer " << id << ": Waiting for data..." << std::endl;
    
    // This will wait until data is available
    auto value = q.wait_and_pop();
    std::cout << "Consumer " << id << ": Got " << *value << std::endl;
}

int main()
{
    threadsafe_queue<int> q;
    
    std::cout << "=== Thread-Safe Queue with Waiting Demo ===" << std::endl;
    
    // Test non-blocking operations first
    std::cout << "\n1. Non-blocking operations:" << std::endl;
    auto result = q.try_pop();
    std::cout << "try_pop on empty queue: " << (result ? "got value" : "no value") << std::endl;
    
    q.push(1);
    q.push(2);
    std::cout << "Pushed: 1, 2" << std::endl;
    
    int value;
    if (q.try_pop(value))
    {
        std::cout << "try_pop got: " << value << std::endl;
    }
    
    // Test blocking operations
    std::cout << "\n2. Blocking operations:" << std::endl;
    
    // Start consumers that will wait
    std::thread consumer1(waiting_consumer, std::ref(q), 1);
    std::thread consumer2(waiting_consumer, std::ref(q), 2);
    
    // Start slow producer
    std::thread producer(slow_producer, std::ref(q));
    
    // Wait for all threads
    consumer1.join();
    consumer2.join();
    producer.join();
    
    std::cout << "\nDemo completed. Key features:" << std::endl;
    std::cout << "- try_pop: Non-blocking, returns immediately" << std::endl;
    std::cout << "- wait_and_pop: Blocking, waits for data" << std::endl;
    std::cout << "- Fine-grained locking for maximum concurrency" << std::endl;
    std::cout << "- Exception-safe operations" << std::endl;
    
    return 0;
}