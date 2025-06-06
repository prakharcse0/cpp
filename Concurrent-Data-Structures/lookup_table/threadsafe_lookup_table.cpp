#include <iostream>
#include <vector>
#include <list>
#include <shared_mutex>
#include <memory>
#include <functional>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <random>

// Complete implementation of thread-safe lookup table from the text
// This is the full implementation from Listing 6.11 with modern C++ features
template<typename Key, typename Value, typename Hash = std::hash<Key>>
class ThreadSafeLookupTable {
private:
    // Individual bucket implementation
    class BucketType {
    private:
        using BucketValue = std::pair<Key, Value>;
        using BucketData = std::list<BucketValue>;
        using BucketIterator = typename BucketData::iterator;
        
        BucketData data;                        // Storage for this bucket
        mutable std::shared_mutex mutex;        // Protection for this bucket
        
        // Find entry in this bucket - returns iterator
        BucketIterator find_entry_for(const Key& key) {
            return std::find_if(data.begin(), data.end(),
                [&](const BucketValue& item) {
                    return item.first == key;
                });
        }
        
        // Const version for read operations
        typename BucketData::const_iterator find_entry_for(const Key& key) const {
            return std::find_if(data.begin(), data.end(),
                [&](const BucketValue& item) {
                    return item.first == key;
                });
        }
        
    public:
        // Get value with default - read operation uses shared lock
        Value value_for(const Key& key, const Value& default_value) const {
            std::shared_lock<std::shared_mutex> lock(mutex);
            auto found_entry = find_entry_for(key);
            return (found_entry == data.end()) ? default_value : found_entry->second;
        }
        
        // Add or update mapping - write operation uses unique lock
        void add_or_update_mapping(const Key& key, const Value& value) {
            std::unique_lock<std::shared_mutex> lock(mutex);
            auto found_entry = find_entry_for(key);
            if (found_entry == data.end()) {
                // Add new entry
                data.push_back(BucketValue(key, value));
            } else {
                // Update existing entry
                found_entry->second = value;
            }
        }
        
        // Remove mapping - write operation uses unique lock
        void remove_mapping(const Key& key) {
            std::unique_lock<std::shared_mutex> lock(mutex);
            auto found_entry = find_entry_for(key);
            if (found_entry != data.end()) {
                data.erase(found_entry);
            }
        }
        
        // For snapshot operations - return copy of all data
        std::list<BucketValue> get_all_data() const {
            std::shared_lock<std::shared_mutex> lock(mutex);
            return data;
        }
        
        // Get exclusive lock for cross-bucket operations
        std::unique_lock<std::shared_mutex> get_lock() {
            return std::unique_lock<std::shared_mutex>(mutex);
        }
    };
    
    std::vector<std::unique_ptr<BucketType>> buckets;  // Bucket array
    Hash hasher;                                       // Hash function
    
    // Get bucket for a given key
    BucketType& get_bucket(const Key& key) const {
        std::size_t bucket_index = hasher(key) % buckets.size();
        return *buckets[bucket_index];
    }
    
public:
    // Type aliases for STL compatibility
    using key_type = Key;
    using mapped_type = Value;
    using hash_type = Hash;
    
    // Constructor with configurable buckets and hash function
    explicit ThreadSafeLookupTable(
        unsigned num_buckets = 19,
        const Hash& hasher_func = Hash{})
        : buckets(num_buckets), hasher(hasher_func) {
        
        // Initialize all buckets
        for (unsigned i = 0; i < num_buckets; ++i) {
            buckets[i] = std::make_unique<BucketType>();
        }
    }
    
    // Delete copy operations for thread safety
    ThreadSafeLookupTable(const ThreadSafeLookupTable&) = delete;
    ThreadSafeLookupTable& operator=(const ThreadSafeLookupTable&) = delete;
    
    // Main interface functions - delegate to appropriate bucket
    Value value_for(const Key& key, const Value& default_value = Value{}) const {
        return get_bucket(key).value_for(key, default_value);
    }
    
    void add_or_update_mapping(const Key& key, const Value& value) {
        get_bucket(key).add_or_update_mapping(key, value);
    }
    
    void remove_mapping(const Key& key) {
        get_bucket(key).remove_mapping(key);
    }
    
    // Snapshot function - implementation from Listing 6.12
    // Locks all buckets in order to prevent deadlock
    std::map<Key, Value> get_map() const {
        // Lock all buckets in order
        std::vector<std::unique_lock<std::shared_mutex>> locks;
        for (unsigned i = 0; i < buckets.size(); ++i) {
            locks.push_back(buckets[i]->get_lock());
        }
        
        // Collect all data from all buckets
        std::map<Key, Value> result;
        for (unsigned i = 0; i < buckets.size(); ++i) {
            auto bucket_data = buckets[i]->get_all_data();
            for (const auto& item : bucket_data) {
                result.insert(item);
            }
        }
        
        return result;
    }
    
    // Utility functions
    size_t bucket_count() const {
        return buckets.size();
    }
    
    // Get approximate size (without locking all buckets)
    size_t approximate_size() const {
        size_t total = 0;
        for (const auto& bucket : buckets) {
            // This is approximate since we're not locking
            total += bucket->get_all_data().size();
        }
        return total;
    }
};

// Demonstration of concurrent usage
void concurrent_writer(ThreadSafeLookupTable<std::string, int>& table, 
                      int thread_id, int operations) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    for (int i = 0; i < operations; ++i) {
        std::string key = "key_" + std::to_string(thread_id) + "_" + std::to_string(i);
        int value = dis(gen);
        table.add_or_update_mapping(key, value);
        
        // Occasional removal
        if (i % 5 == 0 && i > 0) {
            std::string old_key = "key_" + std::to_string(thread_id) + "_" + std::to_string(i-5);
            table.remove_mapping(old_key);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void concurrent_reader(const ThreadSafeLookupTable<std::string, int>& table, 
                      int thread_id, int operations) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 2);
    
    for (int i = 0; i < operations; ++i) {
        // Read from different threads' keys
        int target_thread = dis(gen);
        std::string key = "key_" + std::to_string(target_thread) + "_" + std::to_string(i % 10);
        int value = table.value_for(key, -1);
        
        if (value != -1) {
            std::cout << "Reader " << thread_id << " found " << key << " = " << value << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
}

int main() {
    std::cout << "=== Complete Thread-Safe Lookup Table Demo ===" << std::endl;
    
    // Create lookup table with moderate number of buckets
    ThreadSafeLookupTable<std::string, int> lookup_table(13);
    
    std::cout << "Created lookup table with " << lookup_table.bucket_count() << " buckets" << std::endl;
    
    // Add some initial data
    std::cout << "\n--- Adding initial data ---" << std::endl;
    for (int i = 0; i < 10; ++i) {
        lookup_table.add_or_update_mapping("init_" + std::to_string(i), i * 10);
    }
    
    std::cout << "\n--- Starting concurrent access test ---" << std::endl;
    
    // Create concurrent threads
    std::vector<std::thread> threads;
    
    // Start writer threads
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back(concurrent_writer, std::ref(lookup_table), i, 20);
    }
    
    // Start reader threads
    for (int i = 0; i < 2; ++i) {
        threads.emplace_back(concurrent_reader, std::cref(lookup_table), i + 10, 30);
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "\n--- Final state snapshot ---" << std::endl;
    auto final_state = lookup_table.get_map();
    
    std::cout << "Final table contains " << final_state.size() << " entries:" << std::endl;
    for (const auto& pair : final_state) {
        std::cout << "  " << pair.first << " -> " << pair.second << std::endl;
    }
    
    return 0;
}

/*
Complete Implementation Features:
1. Fine-grained locking per bucket
2. Shared mutex for reader/writer concurrency within buckets
3. Hash-based bucket assignment
4. Exception-safe operations
5. Consistent snapshot functionality
6. STL-compatible interface

Exception Safety Analysis:
- value_for: No modifications, exception-safe
- remove_mapping: Uses erase() which is no-throw, safe
- add_or_update_mapping: push_back is exception-safe, assignment depends on Value type

Performance Characteristics:
- O(1) average case for all operations (good hash function assumed)
- N-fold concurrency improvement (N = bucket count)
- Reader operations can proceed concurrently within same bucket
- Operations on different buckets are fully concurrent

Threading Model:
- Each bucket can have multiple readers OR one writer
- Different buckets can be accessed simultaneously
- Snapshot operation requires exclusive access to all buckets
- Deadlock prevention through consistent bucket locking order
*/