// g++ -std=c++11 03_12_protecting_rarely_updated_data_structures.cpp -lboost_thread -lboost_system

#include <map>
#include <string>
#include <mutex>
#include <boost/thread/shared_mutex.hpp>

// Forward declaration of DNS entry structure
class dns_entry {
    // Contains IP address, TTL, and other DNS data
    // Implementation details omitted for brevity
};

class dns_cache {
private:
    // Core data structure: maps domain names to their DNS entries
    std::map<std::string, dns_entry> entries;
    
    // Reader-writer mutex: allows multiple readers OR single writer
    // 'mutable' because we need to lock in const member functions
    mutable boost::shared_mutex entry_mutex;

public:
    
    // READ operation: allows concurrent access by multiple threads
    dns_entry find_entry(std::string const& domain) const {
        
        // Shared lock: multiple threads can hold this simultaneously
        // Only blocks if another thread has exclusive lock
        boost::shared_lock<boost::shared_mutex> lk(entry_mutex);
        
        // Safe to read from map while holding shared lock
        std::map<std::string, dns_entry>::const_iterator const it = 
            entries.find(domain);
            
        // Return found entry or default-constructed entry if not found
        return (it == entries.end()) ? dns_entry() : it->second;
        
        // Shared lock automatically released when 'lk' goes out of scope
    }
    
    // WRITE operation: requires exclusive access
    void update_or_add_entry(std::string const& domain, 
                           dns_entry const& dns_details) {
        
        // Exclusive lock: blocks ALL other threads (readers and writers)
        // Waits until all shared locks are released before proceeding
        std::lock_guard<boost::shared_mutex> lk(entry_mutex);
        
        // Safe to modify map while holding exclusive lock
        entries[domain] = dns_details;
        
        // Exclusive lock automatically released when 'lk' goes out of scope
    }
};

/*
KEY CONCEPTS EXPLAINED:

1. PROBLEM CONTEXT:
   - DNS cache data is mostly READ, rarely WRITTEN
   - Multiple threads need concurrent READ access
   - Write operations need exclusive access for data integrity
   - std::mutex is too restrictive (blocks all concurrent reads)

2. READER-WRITER MUTEX BEHAVIOR:
   - Multiple threads can hold SHARED locks simultaneously (for reading)
   - Only ONE thread can hold EXCLUSIVE lock (for writing)
   - Exclusive lock blocks until all shared locks are released
   - Shared locks block if exclusive lock is held

3. LOCKING STRATEGY:
   - find_entry(): Uses boost::shared_lock for READ operations
     * Allows multiple concurrent readers
     * Blocks if writer is active
   
   - update_or_add_entry(): Uses std::lock_guard for WRITE operations
     * Blocks all other threads (readers and writers)
     * Ensures data consistency during modifications

4. PERFORMANCE BENEFITS:
   - High read concurrency when no writes occurring
   - Only blocks readers during actual write operations
   - Ideal for read-heavy workloads like DNS caching

5. MUTEX SELECTION:
   - boost::shared_mutex: Supports both shared and exclusive locking
   - std::lock_guard<boost::shared_mutex>: For exclusive access
   - boost::shared_lock<boost::shared_mutex>: For shared access

6. IMPORTANT CONSIDERATIONS:
   - Performance depends on reader/writer ratio and hardware
   - More complex than simple mutex - profile before using
   - C++14 later added std::shared_mutex to standard library
   - Overhead may not be worth it for write-heavy workloads

7. THREAD SAFETY GUARANTEES:
   - Readers see consistent data (no torn reads)
   - Writers have exclusive access (no race conditions)
   - No deadlocks with proper RAII lock management
*/


int main() {

    return 0;
}