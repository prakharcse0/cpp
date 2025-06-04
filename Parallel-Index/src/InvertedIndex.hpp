// project_root/src/InvertedIndex.hpp
#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <shared_mutex> // C++17 for std::shared_mutex

/**
 * @brief Represents an occurrence of a word within a specific file.
 * Includes the file ID and the positions (offsets) where the word appears.
 */
struct WordOccurrence {
    size_t file_id;            ///< The unique ID of the file where the word occurs.
    std::vector<size_t> positions; ///< A list of byte offsets where the word starts in the file.
};

/**
 * @brief A thread-safe inverted index for storing word-to-file mappings.
 *
 * This class allows multiple threads to add word occurrences concurrently during indexing
 * and multiple threads to search for words concurrently. It uses std::shared_mutex
 * to optimize for scenarios with more reads than writes.
 */
class InvertedIndex {
public:
    InvertedIndex() = default;
    ~InvertedIndex() = default;

    // Delete copy constructor and assignment operator to prevent accidental copying
    InvertedIndex(const InvertedIndex&) = delete;
    InvertedIndex& operator=(const InvertedIndex&) = delete;

    /**
     * @brief Adds a word occurrence to the index.
     * This operation is thread-safe (exclusive lock for writes).
     * If the word already exists, it updates the occurrence list for the given file.
     *
     * @param word The word to add.
     * @param file_id The ID of the file where the word occurs.
     * @param position The byte offset of the word within the file.
     */
    void add_word_occurrence(const std::string& word, size_t file_id, size_t position);

    /**
     * @brief Searches for a word in the index.
     * This operation is thread-safe (shared lock for reads).
     *
     * @param word The word to search for.
     * @return A vector of WordOccurrence structures indicating where the word was found.
     */
    std::vector<WordOccurrence> search(const std::string& word) const;

    /**
     * @brief Clears all entries from the index.
     * This operation is thread-safe (exclusive lock).
     */
    void clear();

    /**
     * @brief Returns the total number of unique words in the index.
     * This operation is thread-safe (shared lock).
     *
     * @return The number of unique words.
     */
    size_t size() const;

private:
    // The main index: maps a word to a list of its occurrences in different files.
    // The inner map could store positions for each file_id: std::unordered_map<size_t, std::vector<size_t>>
    // For simplicity and to use the WordOccurrence struct, we'll keep it as a vector of structs.
    std::unordered_map<std::string, std::vector<WordOccurrence>> index_;

    mutable std::shared_mutex mutex_; // Mutex for thread-safe access to 'index_'
};