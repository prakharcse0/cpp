// project_root/src/Utils.cpp
#include "Utils.hpp"
#include <algorithm>  // For std::transform
#include <cctype>     // For std::tolower, std::isalnum
#include <sstream>    // For std::stringstream
#include <string>     // For std::string

namespace Utils {

    // Internal static set for stop words. Protected from external access.
    static std::set<std::string> stop_words_;

    /**
     * @brief Converts a string to lowercase.
     * @param s The input string.
     * @return The lowercase version of the string.
     */
    std::string to_lower(const std::string& s) {
        std::string lower_s = s;
        std::transform(lower_s.begin(), lower_s.end(), lower_s.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        return lower_s;
    }

    /**
     * @brief Tokenizes a given text string into words.
     * Converts words to lowercase and removes non-alphanumeric characters.
     * Words that are defined as stop words are ignored.
     *
     * @param text The input text string.
     * @return A vector of processed word strings.
     */
    std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::stringstream ss(text);
        std::string word;

        while (ss >> word) {
            // Remove non-alphanumeric characters and convert to lowercase
            std::string cleaned_word;
            for (char c : word) {
                if (std::isalnum(static_cast<unsigned char>(c))) {
                    cleaned_word += std::tolower(static_cast<unsigned char>(c));
                }
            }

            if (!cleaned_word.empty()) {
                // Check if it's a stop word
                if (stop_words_.find(cleaned_word) == stop_words_.end()) {
                    tokens.push_back(cleaned_word);
                }
            }
        }
        return tokens;
    }

    /**
     * @brief Sets the stop words to be ignored during tokenization.
     * Words in this set will not be returned by the tokenize function.
     *
     * @param words A set of strings representing the stop words.
     */
    void set_stop_words(const std::set<std::string>& words) {
        stop_words_ = words;
    }

} // namespace Utils