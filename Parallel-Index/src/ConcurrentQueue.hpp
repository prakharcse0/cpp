#pragma once

#include <queue>              // For std::queue
#include <mutex>              // For std::mutex
#include <condition_variable> // For std::condition_variable
#include <optional>           // C++17: For std::optional

/**
 * @brief A thread-safe queue implementation for producer-consumer patterns.
 *
 * This queue supports multiple producers and multiple consumers.
 * It provides blocking and non-blocking pop operations, and a mechanism to signal
 * that no more items will be pushed (closing the queue), allowing consumers to gracefully exit.
 *
 * @tparam T The type of elements to be stored in the queue.
 */

template<typename T>
class ConcurrentQueue {

private:
    std::queue<T> queue_;            // The underlying standard queue
    mutable std::mutex mutex_;       // Mutex to protect access to queue_ and closed_
    std::condition_variable cond_var_; // Condition variable for signaling and waiting
    bool closed_ = false;            // Flag to indicate if the queue is closed for pushing

public:
    // @brief Pushes @param value value onto the back of the queue.
    void push(T value) {
        { // Scope for unique_lock
            std::unique_lock<std::mutex> lock(mutex_);
            queue_.push(std::move(value)); // Use std::move for efficiency if 'value' is an rvalue
        }
        cond_var_.notify_one(); // Notify one waiting consumer that a new item is available
    }


    /**
     * @brief Attempts to pop a value from the front of the queue without blocking.
     * This operation is thread-safe.
     *
     * @return An std::optional<T> containing the popped value if successful,
     * or std::nullopt if the queue is empty or closed.
     */
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mutex_); // Acquire lock
        if (queue_.empty() && closed_) {
            // If queue is empty AND closed, no more items will ever arrive.
            return std::nullopt; // Signal that queue is exhausted
        }
        if (queue_.empty()) {
            // If queue is empty but not closed, a non-blocking pop just returns empty.
            return std::nullopt;
        }

        T value = std::move(queue_.front()); 
        // Since we're about to queue_.pop() and discard the original, moving avoids an unnecessary copy.
        queue_.pop(); // Remove from queue
        return value; // Return as optional
    }


    /**
     * @brief Blocks until a value is available in the queue and then pops it.
     * This operation is thread-safe. If the queue is closed and empty, it will return false.
     *
     * @param value A reference where the popped value will be moved into.
     * @return True if a value was popped, false if the queue was closed and empty.
     */
    bool wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_); // Acquire lock
        // Wait until queue is not empty OR queue is closed
        cond_var_.wait(lock, [this] { return !queue_.empty() || closed_; });

        if (queue_.empty() && closed_) {
            // If we woke up because the queue is closed and it's empty, no more items.
            return false;
        }

        // We know queue is not empty here (otherwise, it must be closed and empty, which we handled)
        value = std::move(queue_.front()); // Move the element
        queue_.pop();
        return true;
    }


    /**
     * @brief Closes the queue, signaling that no more items will be pushed.
     * Consumers waiting on `wait_and_pop` will be notified and can gracefully exit
     * once the queue becomes empty.
     * This operation is thread-safe.
     */
    void close() {
        { // Scope for unique_lock
            std::unique_lock<std::mutex> lock(mutex_); // Acquire lock
            closed_ = true; // Set the flag
        } // Lock is released here
        cond_var_.notify_all(); // Notify all waiting consumers to check the `closed_` flag
    }

    /**
     * @brief Checks if the queue is currently empty.
     * This operation is thread-safe.
     *
     * @return True if the queue is empty, false otherwise.
     */
    bool is_empty() const {
        std::unique_lock<std::mutex> lock(mutex_); // Acquire lock
        return queue_.empty();
    }

    /**
     * @brief Checks if the queue has been closed.
     * This operation is thread-safe.
     *
     * @return True if the queue has been closed, false otherwise.
     */
    bool is_closed() const {
        std::unique_lock<std::mutex> lock(mutex_); // Acquire lock
        return closed_;
    }

    /**
     * @brief Returns the current size of the queue.
     * This operation is thread-safe.
     *
     * @return The number of elements currently in the queue.
     */
    size_t size() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return queue_.size();
    }
};