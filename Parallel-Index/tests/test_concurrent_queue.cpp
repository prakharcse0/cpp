// project_root/tests/test_concurrent_queue.cpp
#include <gtest/gtest.h>
#include "ConcurrentQueue.hpp"
#include <thread> // Changed from <jthread>
#include <vector>
#include <chrono>
#include <atomic> // For total_popped_count


TEST(ConcurrentQueueTest, MultipleProducersConsumers) {
    ConcurrentQueue<int> q;
    const int num_producers = 5;
    const int num_consumers = 5;
    const int items_per_producer = 1000;
    std::atomic<int> total_popped_count = 0;

    // Producers
    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&q, items_per_producer]() {
            for (int j = 0; j < items_per_producer; ++j) {
                q.push(j);
            }
        });
    }

    // Consumers
    std::vector<std::thread> consumers; // Changed from std::jthread
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&q, &total_popped_count]() {
            int val;
            while (q.wait_and_pop(val)) {
                total_popped_count++;
            }
        });
    }

    // Explicitly join producers
    for (auto& p_thread : producers) {
        p_thread.join();
    }
    q.close(); // Signal to consumers that no more items will be pushed

    // Explicitly join consumers
    for (auto& c_thread : consumers) {
        c_thread.join();
    }

    EXPECT_EQ(total_popped_count, num_producers * items_per_producer);
    EXPECT_TRUE(q.is_empty());
    EXPECT_TRUE(q.is_closed());
}

TEST(ConcurrentQueueTest, TimedWaitAndPopNotProvided_UsingWaitAndPop) {
    ConcurrentQueue<int> q;
    std::thread t([&q]() { // Changed from std::jthread
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Give time for consumer to block
        q.close();
    });

    int val;
    bool popped = q.wait_and_pop(val);
    EXPECT_FALSE(popped);
    EXPECT_TRUE(q.is_empty());
    EXPECT_TRUE(q.is_closed());

    t.join(); // Explicitly join the test thread
}