#ifndef SCHEDULER_THREAD_POOL_HPP
#define SCHEDULER_THREAD_POOL_HPP

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <stdexcept>

namespace scheduler {

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    // Submit a task to the thread pool
    template<class F, class... Args>
    auto submit(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

private:
    // Need to keep track of threads so we can join them
    std::vector<std::thread> workers_;
    // The task queue
    std::queue<std::function<void()>> tasks_;

    // Synchronization primitives
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_; // Flag to signal threads to stop
};

} // namespace scheduler

#endif // SCHEDULER_THREAD_POOL_HPP