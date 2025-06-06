#include <array>         // For std::array
#include <functional>    // For std::hash
#include <shared_mutex>  // For std::shared_mutex, std::shared_lock, std::unique_lock
#include <unordered_map> // For std::unordered_map
#include <vector>

template<typename Key, typename Value>
class ConcurrentShardedMap {
private:
    static constexpr size_t NUM_BUCKETS = 16;

    // Now using std::shared_mutex for each bucket.
    // This allows multiple readers but only one writer per bucket.
    std::array<std::shared_mutex, NUM_BUCKETS> bucket_mutexes_;

    std::unordered_map<Key, Value> buckets_[NUM_BUCKETS];

    size_t get_bucket_index(const Key& key) {
        return std::hash<Key>{}(key) % NUM_BUCKETS;
    }

public:
    // Inserts or updates a key-value pair.
    // Uses std::unique_lock for exclusive write access to the bucket.
    void insert(const Key& key, const Value& value) {
        size_t idx = get_bucket_index(key);
        std::unique_lock<std::shared_mutex> lock(bucket_mutexes_[idx]);
        buckets_[idx][key] = value;
    }

    // Attempts to find a value by key.
    // Uses std::shared_lock for shared read access, allowing multiple readers concurrently.
    bool find(const Key& key, Value& value) {
        size_t idx = get_bucket_index(key);
        std::shared_lock<std::shared_mutex> lock(bucket_mutexes_[idx]); // Shared lock
        auto it = buckets_[idx].find(key);
        if (it != buckets_[idx].end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    // Removes a key-value pair.
    // Uses std::unique_lock for exclusive write access.
    bool erase(const Key& key) {
        size_t idx = get_bucket_index(key);
        std::unique_lock<std::shared_mutex> lock(bucket_mutexes_[idx]); // Exclusive lock
        return buckets_[idx].erase(key) > 0;
    }

    // Checks if a key exists in the map.
    // Uses std::shared_lock for shared read access.
    bool contains(const Key& key) {
        size_t idx = get_bucket_index(key);
        std::shared_lock<std::shared_mutex> lock(bucket_mutexes_[idx]); // Shared lock
        return buckets_[idx].count(key) > 0;
    }

    // Checks if the entire map is empty.
    // This still requires acquiring exclusive locks on all buckets for a consistent view,
    // as it's a "global" state check that could be affected by concurrent writes.
    bool empty() {
        // Acquire exclusive locks for all buckets to ensure a consistent view.
        // This effectively serializes this global operation.
        std::array<std::unique_lock<std::shared_mutex>, NUM_BUCKETS> locks;
        for (size_t i = 0; i < NUM_BUCKETS; ++i) {
            locks[i] = std::unique_lock<std::shared_mutex>(bucket_mutexes_[i]);
        }

        for (size_t i = 0; i < NUM_BUCKETS; ++i) {
            if (!buckets_[i].empty()) {
                return false;
            }
        }
        return true;
    }

    // Returns the total number of elements in the map.
    // Also requires acquiring exclusive locks on all buckets for a consistent sum.
    size_t size() {
        // Acquire exclusive locks for all buckets.
        std::array<std::unique_lock<std::shared_mutex>, NUM_BUCKETS> locks;
        for (size_t i = 0; i < NUM_BUCKETS; ++i) {
            locks[i] = std::unique_lock<std::shared_mutex>(bucket_mutexes_[i]);
        }

        size_t total_size = 0;
        for (size_t i = 0; i < NUM_BUCKETS; ++i) {
            total_size += buckets_[i].size();
        }
        return total_size;
    }

    // Clears all elements from the map.
    // Requires acquiring exclusive locks on all buckets.
    void clear() {
        // Acquire exclusive locks for all buckets.
        std::array<std::unique_lock<std::shared_mutex>, NUM_BUCKETS> locks;
        for (size_t i = 0; i < NUM_BUCKETS; ++i) {
            locks[i] = std::unique_lock<std::shared_mutex>(bucket_mutexes_[i]);
        }

        for (size_t i = 0; i < NUM_BUCKETS; ++i) {
            buckets_[i].clear();
        }
    }
};