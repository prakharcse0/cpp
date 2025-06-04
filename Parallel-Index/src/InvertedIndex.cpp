// project_root/src/InvertedIndex.cpp
#include "InvertedIndex.hpp"
#include <algorithm> // For std::find_if
#include <mutex>

/**
 * @brief Adds a word occurrence to the index.
 * This operation is thread-safe (exclusive lock for writes).
 * If the word already exists, it updates the occurrence list for the given file.
 *
 * @param word The word to add.
 * @param file_id The ID of the file where the word occurs.
 * @param position The byte offset of the word within the file.
 */
void InvertedIndex::add_word_occurrence(const std::string& word, size_t file_id, size_t position) {
    // Use std::unique_lock for exclusive access during write operations
    std::unique_lock<std::shared_mutex> lock(mutex_);

    // Find the entry for the given word
    auto& occurrences_for_word = index_[word]; // operator[] handles insertion if word not present

    // Check if an occurrence for this file_id already exists
    auto it = std::find_if(occurrences_for_word.begin(), occurrences_for_word.end(),
                           [file_id](const WordOccurrence& occ) {
                               return occ.file_id == file_id;
                           });

    if (it != occurrences_for_word.end()) {
        // If file_id found, add position to existing occurrence
        it->positions.push_back(position);
    } else {
        // If file_id not found, create a new WordOccurrence entry
        occurrences_for_word.push_back({file_id, {position}});
    }
}

/**
 * @brief Searches for a word in the index.
 * This operation is thread-safe (shared lock for reads).
 *
 * @param word The word to search for.
 * @return A vector of WordOccurrence structures indicating where the word was found.
 */
std::vector<WordOccurrence> InvertedIndex::search(const std::string& word) const {
    // Use std::shared_lock for shared access during read operations
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = index_.find(word);
    if (it != index_.end()) {
        return it->second; // Return the vector of occurrences
    }
    return {}; // Return an empty vector if word not found
}

/**
 * @brief Clears all entries from the index.
 * This operation is thread-safe (exclusive lock).
 */
void InvertedIndex::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    index_.clear();
}

/**
 * @brief Returns the total number of unique words in the index.
 * This operation is thread-safe (shared lock).
 *
 * @return The number of unique words.
 */
size_t InvertedIndex::size() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return index_.size();
}