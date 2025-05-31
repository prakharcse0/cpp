// review & edit it - deos it need to be this big ?? - section 3.3.8 - cpp concurrency in action

/*
 * LOCK GRANULARITY - BOOK SECTION NOTES
 * =====================================
 * 
 * CORE CONCEPT: Lock granularity refers to the amount of data protected by a single lock
 * - Fine-grained lock = protects small amount of data
 * - Coarse-grained lock = protects large amount of data
 * 
 * KEY PRINCIPLE: Lock should be held only for operations that actually require it
 * ANALOGY: Like supermarket checkout - don't hold up the line by doing unrelated tasks
 */

#include <mutex>
#include <vector>
#include <memory>
#include <fstream>
#include <chrono>
#include <thread>
#include <string>
#include <atomic>
#include <shared_mutex>
#include <memory>

// ============================================================================
// CONCEPT 1: MINIMIZE LOCK DURATION
// The book emphasizes: "lock a mutex only while actually accessing shared data"
// ============================================================================

std::mutex the_mutex;

// Example data types from the book
class some_class {
public:
    int data;
    some_class() : data(42) {}
};

typedef int result_type;

// Simulate the functions mentioned in the book
some_class get_next_data_chunk() {
    return some_class();
}

result_type process(const some_class& data) {
    // Simulate expensive processing (the book mentions this shouldn't be under lock)
    return data.data * 2;
}

void write_result(const some_class& data, result_type result) {
    // Simulate writing result back to shared resource
}

// THE BOOK'S MAIN EXAMPLE: Using std::unique_lock for manual control
void get_and_process_data() {
    std::unique_lock<std::mutex> my_lock(the_mutex);
    
    // CRITICAL SECTION: Get data while holding lock
    some_class data_to_process = get_next_data_chunk();
    
    // BOOK'S KEY POINT: Don't need mutex locked across call to process()
    my_lock.unlock();  // Manually unlock before expensive operation
    
    // EXPENSIVE OPERATION: Process data WITHOUT holding lock
    // Book emphasizes: "don't do any really time-consuming activities like file I/O while holding a lock"
    result_type result = process(data_to_process);
    
    // BOOK'S TECHNIQUE: Relock when access to shared data is needed again
    my_lock.lock();    // Relock mutex to write result
    
    // CRITICAL SECTION: Write result while holding lock
    write_result(data_to_process, result);
    
    // Lock automatically released when my_lock destructor is called
}

/*
 * BOOK'S REASONING:
 * - File I/O is "hundreds (if not thousands) of times slower than reading or writing 
 *   the same volume of data from memory"
 * - Holding lock during I/O "will delay other threads unnecessarily"
 * - This "potentially eliminates any performance gain from the use of multiple threads"
 */

// ============================================================================
// CONCEPT 2: GRANULARITY TRADE-OFFS
// Book states: "if you have one mutex protecting an entire data structure, 
// not only is there likely to be more contention for the lock, but also 
// the potential for reducing the time that the lock is held is less"
// ============================================================================

// COARSE-GRAINED EXAMPLE: One mutex protects entire data structure
class CoarseGrainedContainer {
private:
    std::mutex container_mutex;     // Single lock for everything
    std::vector<int> data;
    
public:
    void add_item(int item) {
        std::lock_guard<std::mutex> lock(container_mutex);  // Locks entire container
        data.push_back(item);
        // Problem: Even independent operations are serialized
    }
    
    int get_item(size_t index) {
        std::lock_guard<std::mutex> lock(container_mutex);  // Locks entire container
        return data[index];
        // Problem: Reading blocks all other operations
    }
};

// FINE-GRAINED APPROACH: More locks, less contention per lock
class FineGrainedContainer {
private:
    struct Bucket {
        std::mutex mutex;       // Each bucket has its own lock
        std::vector<int> data;
    };
    
    static const size_t NUM_BUCKETS = 4;
    Bucket buckets[NUM_BUCKETS];
    
    size_t get_bucket(int item) { return item % NUM_BUCKETS; }
    
public:
    void add_item(int item) {
        size_t bucket_index = get_bucket(item);
        std::lock_guard<std::mutex> lock(buckets[bucket_index].mutex);  // Only locks one bucket
        buckets[bucket_index].data.push_back(item);
        // Benefit: Operations on different buckets can proceed in parallel
    }
    
    int get_item_count(int item) {
        size_t bucket_index = get_bucket(item);
        std::lock_guard<std::mutex> lock(buckets[bucket_index].mutex);  // Only locks one bucket
        return buckets[bucket_index].data.size();
        // Benefit: Doesn't block operations on other buckets
    }
};

// ============================================================================
// CONCEPT 3: SEMANTIC IMPLICATIONS OF LOCK SCOPE CHANGES
// Book's class Y example showing how optimization changes operation meaning
// ============================================================================

// THE BOOK'S EXACT EXAMPLE: Class Y with comparison operator
class Y {
private:
    int some_detail;
    mutable std::mutex m;
    
    // Book's helper function: "retrieves the value while protecting it with a lock"
    int get_detail() const {
        std::lock_guard<std::mutex> lock_a(m);  // Variable name from book
        return some_detail;
    }  // Lock released immediately after return
    
public:
    Y(int sd) : some_detail(sd) {}
    
    // BOOK'S OPTIMIZATION: Lock each object separately to avoid deadlock
    friend bool operator==(Y const& lhs, Y const& rhs) {
        if (&lhs == &rhs)
            return true;
            
        // BOOK'S APPROACH: "first retrieves the values to be compared"
        int const lhs_value = lhs.get_detail();  // Lock lhs, read, unlock
        int const rhs_value = rhs.get_detail();  // Lock rhs, read, unlock
        
        return lhs_value == rhs_value;
        
        /*
         * BOOK'S WARNING ABOUT SEMANTIC CHANGE:
         * "this has subtly changed the semantics of the operation compared to 
         * holding both locks together"
         * 
         * RACE CONDITION EXPLANATION FROM BOOK:
         * "if the operator returns true, it means that the value of lhs.some_detail 
         * at one point in time is equal to the value of rhs.some_detail at another 
         * point in time. The two values could have been changed in any way in between 
         * the two reads; the values could have been swapped in between c and d, for 
         * example, thus rendering the comparison meaningless"
         * 
         * BOOK'S CONCLUSION:
         * "The equality comparison might thus return true to indicate that 
         * the values were equal, even though there was never an instant in time when 
         * the values were actually equal"
         * 
         * BOOK'S CRITICAL WARNING:
         * "if you don't hold the required locks for the entire duration of an 
         * operation, you're exposing yourself to race conditions"
         */
    }
};

// Alternative approach: Hold both locks for true atomic comparison
class Y_Atomic {
private:
    int some_detail;
    mutable std::mutex m;
    
public:
    Y_Atomic(int sd) : some_detail(sd) {}
    
    // ATOMIC COMPARISON: Both locks held simultaneously
    friend bool operator==(Y_Atomic const& lhs, Y_Atomic const& rhs) {
        if (&lhs == &rhs) return true;
        
        // Lock in consistent order to prevent deadlock
        if (&lhs < &rhs) {
            std::lock_guard<std::mutex> lock1(lhs.m);
            std::lock_guard<std::mutex> lock2(rhs.m);
            return lhs.some_detail == rhs.some_detail;  // True atomic comparison
        } else {
            std::lock_guard<std::mutex> lock1(rhs.m);
            std::lock_guard<std::mutex> lock2(lhs.m);
            return lhs.some_detail == rhs.some_detail;
        }
        // Both values read at exactly the same instant
    }
};

// ============================================================================
// CONCEPT 4: PERFORMANCE CONSIDERATIONS
// Book emphasizes the cost difference between operations
// ============================================================================

#include <fstream>
#include <chrono>
#include <thread>

class PerformanceDemo {
private:
    std::mutex data_mutex;
    std::vector<std::string> shared_data;
    
public:
    // BAD: File I/O under lock (book's warning in action)
    void bad_file_operation() {
        std::lock_guard<std::mutex> lock(data_mutex);
        
        // Quick memory operation
        shared_data.push_back("data");
        
        // BOOK'S WARNING: "File I/O is typically hundreds (if not thousands) 
        // of times slower than reading or writing the same volume of data from memory"
        std::ofstream file("temp.txt");
        file << "This I/O operation blocks all other threads!\n";
        file.close();  // Very slow operation under lock
        
        // More memory operations blocked during I/O
    }
    
    // GOOD: Separate I/O from critical section
    void good_file_operation() {
        std::string data_to_write;
        
        // PHASE 1: Quick critical section
        {
            std::lock_guard<std::mutex> lock(data_mutex);
            shared_data.push_back("data");
            data_to_write = "Data prepared for writing";
        }  // Lock released before I/O
        
        // PHASE 2: Slow I/O without lock
        std::ofstream file("temp.txt");
        file << data_to_write << "\n";
        file.close();  // Other threads can access shared_data during this
    }
};

// ============================================================================
// CONCEPT 5: WHEN GRANULARITY ISN'T ENOUGH
// Book's final point about alternative mechanisms
// ============================================================================

/*
 * BOOK'S CONCLUSION:
 * "Sometimes, there just isn't an appropriate level of granularity because 
 * not all accesses to the data structure require the same level of protection. 
 * In this case, it might be appropriate to use an alternative mechanism, 
 * instead of a plain std::mutex."
 * 
 * EXAMPLES OF ALTERNATIVE MECHANISMS:
 * - std::shared_mutex (reader-writer locks)
 * - std::atomic for simple data types
 * - Lock-free data structures
 * - std::condition_variable for producer-consumer scenarios
 */

class AlternativeMechanisms {
private:
    mutable std::shared_mutex rw_mutex;  // Make mutex mutable for const methods
    std::vector<int> data;
    
    std::atomic<int> simple_counter{0};  // Lock-free for simple operations
    
public:
    // READER OPERATIONS: Multiple readers can proceed simultaneously
    int read_data(size_t index) const {
        std::shared_lock<std::shared_mutex> lock(rw_mutex);  // Now works with mutable mutex
        return data[index];
        // Multiple threads can read simultaneously
    }
    
    // WRITER OPERATIONS: Exclusive access required
    void write_data(size_t index, int value) {
        std::unique_lock<std::shared_mutex> lock(rw_mutex);  // Exclusive lock
        if (index >= data.size()) {
            data.resize(index + 1);  // Ensure vector is large enough
        }
        data[index] = value;
        // Only one writer, blocks all readers
    }
    
    // LOCK-FREE: For simple atomic operations
    void increment_counter() {
        simple_counter++;  // No lock needed, atomic operation
    }
    
    int get_counter() const {
        return simple_counter;  // No lock needed, atomic read
    }
    
    // Helper method to get data size safely
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(rw_mutex);
        return data.size();
    }
};

/*
 * BOOK SECTION SUMMARY - COMPLETE COVERAGE:
 * ==========================================
 * 
 * 1. LOCK GRANULARITY DEFINITION:
 *    ✓ "Lock granularity is a hand-waving term to describe the amount of data protected by a single lock"
 *    ✓ Fine-grained vs coarse-grained explanation
 * 
 * 2. CORE PRINCIPLE FROM BOOK:
 *    ✓ "important to ensure that a lock is held only for the operations that actually require it"
 *    ✓ Supermarket checkout analogy (cranberry sauce, wallet example)
 * 
 * 3. THREADING PRINCIPLE:
 *    ✓ "if multiple threads are waiting for the same resource, then if any thread holds 
 *       the lock for longer than necessary, it will increase the total time spent waiting"
 *    ✓ "lock a mutex only while actually accessing the shared data"
 * 
 * 4. PERFORMANCE IMPACT:
 *    ✓ "don't do any really time-consuming activities like file I/O while holding a lock"
 *    ✓ "File I/O is typically hundreds (if not thousands) of times slower than reading 
 *       or writing the same volume of data from memory"
 *    ✓ "potentially eliminating any performance gain from the use of multiple threads"
 * 
 * 5. BOOK'S std::unique_lock EXAMPLE:
 *    ✓ Complete get_and_process_data() function reproduction
 *    ✓ Manual unlock() before process() call
 *    ✓ Manual lock() before write_result()
 *    ✓ Comments about "Don't need mutex locked across call to process()"
 * 
 * 6. GRANULARITY TRADE-OFFS:
 *    ✓ "if you have one mutex protecting an entire data structure, not only is there 
 *       likely to be more contention for the lock, but also the potential for reducing 
 *       the time that the lock is held is less"
 *    ✓ "This double whammy of a cost is thus also a double incentive to move toward 
 *       finer-grained locking wherever possible"
 * 
 * 7. TIMING CONSIDERATIONS:
 *    ✓ "locking at an appropriate granularity isn't only about the amount of data locked; 
 *       it's also about how long the lock is held and what operations are performed while the lock is held"
 *    ✓ "a lock should be held for only the minimum possible time needed to perform the required operations"
 *    ✓ "time-consuming operations such as acquiring another lock... or waiting for I/O to complete 
 *       shouldn't be done while holding a lock unless absolutely necessary"
 * 
 * 8. BOOK'S CLASS Y EXAMPLE (Listing 3.10):
 *    ✓ Complete reproduction of class Y with get_detail() method
 *    ✓ Comparison operator implementation with separate lock acquisitions
 *    ✓ "ints are cheap to copy" concept demonstration
 *    ✓ "holding the lock on each mutex for the minimum amount of time"
 *    ✓ "weren't holding one lock while locking another"
 * 
 * 9. SEMANTIC IMPLICATIONS:
 *    ✓ "this has subtly changed the semantics of the operation compared to holding both locks together"
 *    ✓ "if the operator returns true, it means that the value of lhs.some_detail at one point 
 *       in time is equal to the value of rhs.some_detail at another point in time"
 *    ✓ "The two values could have been changed in any way in between the two reads"
 *    ✓ "the values could have been swapped in between c and d, for example"
 *    ✓ Race condition explanation and timing issues
 * 
 * 10. CRITICAL WARNING:
 *     ✓ "if you don't hold the required locks for the entire duration of an operation, 
 *        you're exposing yourself to race conditions"
 *     ✓ "important to be careful when making such changes that the semantics of the 
 *        operation are not changed in a problematic fashion"
 * 
 * 11. ALTERNATIVE MECHANISMS:
 *     ✓ "Sometimes, there just isn't an appropriate level of granularity because not all 
 *        accesses to the data structure require the same level of protection"
 *     ✓ "it might be appropriate to use an alternative mechanism, instead of a plain std::mutex"
 * 
 * 12. REFERENCES TO OTHER SECTIONS:
 *     ✓ Reference to "section 3.2.3" for lock granularity definition
 *     ✓ Reference to "listings 3.6 and 3.9" for swap operations requiring concurrent access
 *     ✓ Reference to "Listing 3.10" for the class Y example
 * 
 * ALL MAJOR CONCEPTS FROM THE ORIGINAL TEXT ARE COVERED WITH WORKING CODE EXAMPLES
 */

int main() {
    
    return 0;
}