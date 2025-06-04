// project_root/src/Utils.hpp
#pragma once

#include <string>
#include <vector>
#include <set> // For optional stop words

/**
 * @brief Namespace for general utility functions related to text processing.
 */
namespace Utils {

    /**
     * @brief Converts a string to lowercase.
     * @param s The input string.
     * @return The lowercase version of the string.
     */
    std::string to_lower(const std::string& s);

    /**
     * @brief Tokenizes a given text string into words.
     * Converts words to lowercase and removes non-alphanumeric characters.
     *
     * @param text The input text string.
     * @return A vector of processed word strings.
     */
    std::vector<std::string> tokenize(const std::string& text);

    /**
     * @brief Sets the stop words to be ignored during tokenization.
     * Words in this set will not be returned by the tokenize function.
     *
     * @param words A set of strings representing the stop words.
     */
    void set_stop_words(const std::set<std::string>& words);

} // namespace Utils