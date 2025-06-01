// shared_mutex
// std::shared_lock<std::shared_mutex>
// std::lock_guard<std::shared_mutex>
// std::unique_lock<std::shared_mutex>

#include <map>
#include <string>
#include <shared_mutex>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

/*
SHARED_MUTEX EXPLANATION:

1. THE FUNDAMENTAL PROBLEM:
   - DNS cache is read 1000x more than it's written
   - Regular std::mutex forces ALL access to be sequential (one thread at a time)
   - This wastes CPU cores - readers don't conflict with each other!

2. SOLUTION: std::shared_mutex provides TWO types of locks:
   
   SHARED LOCKS (for readers):
   - Multiple threads can hold shared locks simultaneously
   - Perfect for operations that only READ data
   - No data races because nobody is modifying anything
   
   EXCLUSIVE LOCKS (for writers):
   - Only ONE thread can hold exclusive lock
   - Blocks until ALL shared locks are released
   - Gives complete exclusive access for safe modifications

3. LOCKING RULES:
   
   When NO locks are held:
   ✅ Readers can acquire shared locks
   ✅ Writers can acquire exclusive locks
   
   When SHARED locks are held:
   ✅ More readers can acquire shared locks (CONCURRENT READS)
   ❌ Writers must wait (no exclusive access while readers active)
   
   When EXCLUSIVE lock is held:
   ❌ Nobody else can acquire any lock (complete exclusion)

4. PERFORMANCE BENEFITS:
   
   Traditional mutex approach:
   Thread 1: Read  |-----|
   Thread 2: Read        |-----|  
   Thread 3: Read              |-----|
   Thread 4: Write                   |-----|
   
   Shared_mutex approach:
   Thread 1: Read  |-----|
   Thread 2: Read  |-----|  (CONCURRENT!)
   Thread 3: Read  |-----|  (CONCURRENT!)
   Thread 4: Write       |-----|  (waits for readers)

5. REAL-WORLD SCENARIOS WHERE THIS SHINES:
   - Configuration data (read frequently, updated rarely)
   - Lookup tables and caches
   - Reference data structures
   - Game state that's mostly queried
   - Database query result caches

6. WHEN NOT TO USE:
   - Write-heavy workloads (overhead not worth it)
   - Simple data where regular mutex is sufficient
   - Very short critical sections

7. IMPORTANT GOTCHAS:
   - Writers can be starved if readers keep coming
   - More complex than regular mutex (profile first!)
   - Slightly higher overhead than regular mutex for writes
   - Need C++17 or later

8. LOCK TYPES SUMMARY:
   std::shared_lock<std::shared_mutex>    // For readers (multiple allowed)
   std::lock_guard<std::shared_mutex>     // For writers (exclusive access)
   std::unique_lock<std::shared_mutex>    // For writers (more flexibility)
*/


// Forward declaration - represents DNS entry data
class dns_entry {
    // Contains IP address, TTL, etc. - implementation not shown for brevity
};


class dns_cache {
private:
    // Core data: maps domain names to DNS entries
    std::map<std::string, dns_entry> entries;
    
    // Reader-writer mutex: enables concurrent reads, exclusive writes
    // 'mutable' allows locking in const methods
    mutable std::shared_mutex entry_mutex;

public:
    // READ operation: allows multiple concurrent threads
    dns_entry find_entry(std::string const& domain) const {
        
        // SHARED LOCK: multiple threads can acquire this simultaneously
        // Blocks only if another thread holds exclusive lock
        std::shared_lock<std::shared_mutex> lk(entry_mutex);
        
        // Safe to read map while holding shared lock
        // Multiple readers can execute this code concurrently
        std::map<std::string, dns_entry>::const_iterator const it = 
            entries.find(domain);
            
        return (it == entries.end()) ? dns_entry() : it->second;
        
        // Shared lock automatically released (RAII)
    }
    
    // WRITE operation: requires exclusive access
    void update_or_add_entry(std::string const& domain, 
                           dns_entry const& dns_details) {
        
        // EXCLUSIVE LOCK: blocks ALL other threads (readers AND writers)
        // Waits until all shared locks are released before proceeding
        std::lock_guard<std::shared_mutex> lk(entry_mutex);
        
        // Safe to modify map - we have exclusive access
        entries[domain] = dns_details;
        
        // Exclusive lock automatically released (RAII)
    }
};


int main() {
    // Example usage matching original text's DNS cache concept
    dns_cache cache;
    
    // Write operation - exclusive access
    cache.update_or_add_entry("example.com", dns_entry{});
    
    // Read operations - can happen concurrently
    dns_entry result = cache.find_entry("example.com");
    
    return 0;
}

/*
KEY INSIGHTS FROM ORIGINAL TEXT:
===============================

MOTIVATION:
- DNS entries remain unchanged for long periods (often years)
- Updates are rare but still possible
- Multiple threads need concurrent read access
- Regular std::mutex eliminates beneficial read concurrency

PERFORMANCE CHARACTERISTICS:
- Shared_mutex overhead depends on processor count and workload ratio
- Profile on target system to ensure actual benefit
- More complex than regular mutex - use only when justified

LOCK TYPES USED:
- std::shared_lock<std::shared_mutex>: For shared/read access
- std::lock_guard<std::shared_mutex>: For exclusive/write access
- Both provide RAII (automatic unlock when scope ends)

COMPILATION:
- Requires C++17: compile with -std=c++17
- No external dependencies (part of standard library)
*/
