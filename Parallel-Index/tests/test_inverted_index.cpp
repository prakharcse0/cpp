// project_root/tests/test_inverted_index.cpp
#include <gtest/gtest.h>
#include "InvertedIndex.hpp"
#include <thread> // Keep std::thread
#include <vector>
#include <set>

TEST(InvertedIndexTest, AddAndSearchSingleWord) {
    InvertedIndex index;
    index.add_word_occurrence("hello", 1, 0);
    index.add_word_occurrence("world", 1, 6);
    index.add_word_occurrence("hello", 2, 0);

    std::vector<WordOccurrence> results = index.search("hello");
    ASSERT_EQ(results.size(), 2);

    // Verify results for file 1
    auto it1 = std::find_if(results.begin(), results.end(), [](const WordOccurrence& occ){ return occ.file_id == 1; });
    ASSERT_NE(it1, results.end());
    EXPECT_EQ(it1->file_id, 1);
    ASSERT_EQ(it1->positions.size(), 1);
    EXPECT_EQ(it1->positions[0], 0);

    // Verify results for file 2
    auto it2 = std::find_if(results.begin(), results.end(), [](const WordOccurrence& occ){ return occ.file_id == 2; });
    ASSERT_NE(it2, results.end());
    EXPECT_EQ(it2->file_id, 2);
    ASSERT_EQ(it2->positions.size(), 1);
    EXPECT_EQ(it2->positions[0], 0);

    std::vector<WordOccurrence> world_results = index.search("world");
    ASSERT_EQ(world_results.size(), 1);
    EXPECT_EQ(world_results[0].file_id, 1);
    ASSERT_EQ(world_results[0].positions.size(), 1);
    EXPECT_EQ(world_results[0].positions[0], 6);

    EXPECT_TRUE(index.search("nonexistent").empty());
}

TEST(InvertedIndexTest, AddMultipleOccurrencesSameFile) {
    InvertedIndex index;
    index.add_word_occurrence("test", 10, 5);
    index.add_word_occurrence("test", 10, 15);
    index.add_word_occurrence("test", 10, 25);

    std::vector<WordOccurrence> results = index.search("test");
    ASSERT_EQ(results.size(), 1); // Only one entry for file_id 10
    EXPECT_EQ(results[0].file_id, 10);
    ASSERT_EQ(results[0].positions.size(), 3);
    EXPECT_EQ(results[0].positions[0], 5);
    EXPECT_EQ(results[0].positions[1], 15);
    EXPECT_EQ(results[0].positions[2], 25);
}

TEST(InvertedIndexTest, ConcurrentAdditions) {
    InvertedIndex index;
    const int num_threads = 8;
    const int words_per_thread = 1000;

    std::vector<std::thread> threads; // Changed from std::jthread
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&index, i, words_per_thread]() {
            for (int j = 0; j < words_per_thread; ++j) {
                std::string word = "word_" + std::to_string(j);
                index.add_word_occurrence(word, i, j); // file_id is thread_id
            }
        });
    }

    // Explicitly join all threads, as std::thread doesn't auto-join like std::jthread
    for (auto& t : threads) {
        t.join();
    }

    // Verify unique words
    EXPECT_EQ(index.size(), words_per_thread);

    // Verify occurrences
    for (int j = 0; j < words_per_thread; ++j) {
        std::string word = "word_" + std::to_string(j);
        std::vector<WordOccurrence> results = index.search(word);
        ASSERT_EQ(results.size(), num_threads) << "Word: " << word; // Should be found in all files
        for (int i = 0; i < num_threads; ++i) {
            auto it = std::find_if(results.begin(), results.end(), [i](const WordOccurrence& occ){ return occ.file_id == static_cast<size_t>(i); });
            ASSERT_NE(it, results.end()) << "Word: " << word << ", File ID: " << i;
            ASSERT_EQ(it->positions.size(), 1);
            EXPECT_EQ(it->positions[0], j); // Position should match original j
        }
    }
}

TEST(InvertedIndexTest, ClearIndex) {
    InvertedIndex index;
    index.add_word_occurrence("a", 1, 1);
    index.add_word_occurrence("b", 2, 2);
    ASSERT_EQ(index.size(), 2);
    index.clear();
    EXPECT_EQ(index.size(), 0);
    EXPECT_TRUE(index.search("a").empty());
}