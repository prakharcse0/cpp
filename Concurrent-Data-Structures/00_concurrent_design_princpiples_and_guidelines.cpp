#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>
#include <iostream>
#include <chrono>

// Demonstration of different concurrency design principles

// 1. SERIALIZATION vs TRUE CONCURRENCY
class SerializedCounter {
private:
    int count = 0;
    mutable std::mutex m;
    
public:
    // All operations are serialized - only one thread can operate at a time
    void increment() {
        std::lock_guard<std::mutex> lock(m);
        ++count;  // Critical section
    }
    
    int get() const {
        std::lock_guard<std::mutex> lock(m);
        return count;  // Even reads are serialized
    }
};

// 2. READER-WRITER CONCURRENCY - allowing multiple readers
class ReadWriteCounter {
private:
    int count = 0;
    mutable std::shared_mutex m;  // Allows shared (read) and exclusive (write) access
    
public:
    void increment() {
        std::unique_lock<std::shared_mutex> lock(m);  // Exclusive lock for writing
        ++count;
    }
    
    int get() const {
        std::shared_lock<std::shared_mutex> lock(m);  // Shared lock for reading
        return count;  // Multiple readers can access simultaneously
    }
};

// 3. MINIMIZING LOCK SCOPE - reducing serialization
class OptimizedOperations {
private:
    std::vector<int> data;
    mutable std::mutex m;
    
public:
    // BAD: Large critical section
    void bad_process_and_add(int value) {
        std::lock_guard<std::mutex> lock(m);
        
        // Expensive computation inside lock - blocks other threads
        int processed = 0;
        for(int i = 0; i < 1000; ++i) {
            processed += value * i;
        }
        
        data.push_back(processed);
    }
    
    // GOOD: Minimize lock scope
    void good_process_and_add(int value) {
        // Do expensive work OUTSIDE the lock
        int processed = 0;
        for(int i = 0; i < 1000; ++i) {
            processed += value * i;
        }
        
        // Only lock for the minimum necessary operation
        {
            std::lock_guard<std::mutex> lock(m);
            data.push_back(processed);  // Quick operation under lock
        }
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(m);
        return data.size();
    }
};

// 4. AVOIDING DEADLOCK - lock ordering and scope
class DeadlockDemo {
private:
    mutable std::mutex m1, m2;
    int data1 = 0, data2 = 0;
    
public:
    // BAD: Potential deadlock if called simultaneously with different order
    void bad_transfer_1_to_2() {
        std::lock_guard<std::mutex> lock1(m1);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));  // Simulate work
        std::lock_guard<std::mutex> lock2(m2);
        
        data1--;
        data2++;
    }
    
    void bad_transfer_2_to_1() {
        std::lock_guard<std::mutex> lock2(m2);  // Different lock order!
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::lock_guard<std::mutex> lock1(m1);  // Potential deadlock here
        
        data2--;
        data1++;
    }
    
    // GOOD: Consistent lock ordering prevents deadlock
    void good_transfer_1_to_2() {
        std::lock(m1, m2);  // Lock both simultaneously
        std::lock_guard<std::mutex> lock1(m1, std::adopt_lock);
        std::lock_guard<std::mutex> lock2(m2, std::adopt_lock);
        
        data1--;
        data2++;
    }
    
    void good_transfer_2_to_1() {
        std::lock(m1, m2);  // Same order - no deadlock possible
        std::lock_guard<std::mutex> lock1(m1, std::adopt_lock);
        std::lock_guard<std::mutex> lock2(m2, std::adopt_lock);
        
        data2--;
        data1++;
    }
    
    std::pair<int, int> get_values() const {
        std::lock(m1, m2);
        std::lock_guard<std::mutex> lock1(m1, std::adopt_lock);
        std::lock_guard<std::mutex> lock2(m2, std::adopt_lock);
        return {data1, data2};
    }
};

// 5. INVARIANT PRESERVATION - ensuring data structure consistency
class BankAccount {
private:
    double balance = 1000.0;
    mutable std::mutex m;
    
    // Invariant: balance >= 0 (no overdrafts allowed)
    
public:
    bool withdraw(double amount) {
        std::lock_guard<std::mutex> lock(m);
        
        // Check invariant before modification
        if (balance >= amount) {
            balance -= amount;
            // Invariant still holds: balance >= 0
            return true;
        }
        
        // Don't modify if it would break invariant
        return false;
    }
    
    void deposit(double amount) {
        std::lock_guard<std::mutex> lock(m);
        
        // This operation can never break the invariant
        balance += amount;
    }
    
    double get_balance() const {
        std::lock_guard<std::mutex> lock(m);
        return balance;
    }
};

// Performance comparison function
void performance_test() {
    std::cout << "\n=== Performance Comparison ===" << std::endl;
    
    const int NUM_THREADS = 4;
    const int OPERATIONS_PER_THREAD = 1000;
    
    // Test serialized counter
    {
        SerializedCounter counter;
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        for(int i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back([&counter, OPERATIONS_PER_THREAD]() {
                for(int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                    counter.increment();
                }
            });
        }
        
        for(auto& t : threads) t.join();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Serialized counter: " << duration.count() 
                  << "ms, final count: " << counter.get() << std::endl;
    }
    
    // Test reader-writer counter with mixed operations
    {
        ReadWriteCounter counter;
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        
        // One writer thread
        threads.emplace_back([&counter, OPERATIONS_PER_THREAD]() {
            for(int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                counter.increment();
            }
        });
        
        // Multiple reader threads
        for(int i = 0; i < NUM_THREADS - 1; ++i) {
            threads.emplace_back([&counter, OPERATIONS_PER_THREAD]() {
                for(int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                    volatile int value = counter.get();  // Readers can run concurrently
                    (void)value;  // Suppress unused variable warning
                }
            });
        }
        
        for(auto& t : threads) t.join();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "Reader-writer counter: " << duration.count() 
                  << "ms, final count: " << counter.get() << std::endl;
    }
}

int main() {
    std::cout << "=== Concurrency Design Principles Demo ===" << std::endl;
    
    // Demonstrate lock scope optimization
    std::cout << "\n--- Lock Scope Optimization ---" << std::endl;
    OptimizedOperations ops;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<std::thread> threads;
    for(int i = 0; i < 4; ++i) {
        threads.emplace_back([&ops, i]() {
            for(int j = 0; j < 10; ++j) {
                ops.good_process_and_add(i * 10 + j);
            }
        });
    }
    
    for(auto& t : threads) t.join();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Optimized operations completed in " << duration.count() 
              << "ms, processed " << ops.size() << " items" << std::endl;
    
    // Demonstrate deadlock prevention
    std::cout << "\n--- Deadlock Prevention ---" << std::endl;
    DeadlockDemo demo;
    
    std::thread t1([&demo]() {
        for(int i = 0; i < 100; ++i) {
            demo.good_transfer_1_to_2();
        }
    });
    
    std::thread t2([&demo]() {
        for(int i = 0; i < 100; ++i) {
            demo.good_transfer_2_to_1();
        }
    });
    
    t1.join();
    t2.join();
    
    auto values = demo.get_values();
    std::cout << "Final values: data1=" << values.first 
              << ", data2=" << values.second << std::endl;
    
    // Demonstrate invariant preservation
    std::cout << "\n--- Invariant Preservation ---" << std::endl;
    BankAccount account;
    
    std::cout << "Initial balance: $" << account.get_balance() << std::endl;
    
    if(account.withdraw(500)) {
        std::cout << "Withdrew $500, balance: $" << account.get_balance() << std::endl;
    }
    
    if(!account.withdraw(600)) {
        std::cout << "Failed to withdraw $600 (would break invariant)" << std::endl;
        std::cout << "Balance remains: $" << account.get_balance() << std::endl;
    }
    
    performance_test();
    
    return 0;
}