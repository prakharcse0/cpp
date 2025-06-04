// project_root/src/IndexerWorker.cpp
#include "IndexerWorker.hpp"
#include <string>
#include <sstream> // For std::stringstream
#include <thread>

/**
 * @brief Constructs an IndexerWorker.
 * @param file_queue A reference to the ConcurrentQueue from which to pull files.
 * @param index A reference to the InvertedIndex to update.
 */
IndexerWorker::IndexerWorker(ConcurrentQueue<FileInfo>& file_queue, InvertedIndex& index)
    : file_queue_(file_queue), index_(index) {}

/**
 * @brief The main execution logic for the worker thread.
 * This method should be called by std::jthread or std::thread.
 * It continuously processes files until the queue is closed and empty.
 */
void IndexerWorker::operator()() {
    FileInfo file_info;
    std::cout << "[Worker " << std::this_thread::get_id() << "] Starting..." << std::endl;
    while (file_queue_.wait_and_pop(file_info)) { // Blocking pop until item or queue closed+empty
        // std::cout << "[Worker " << std::this_thread::get_id() << "] Processing file: " << file_info.path << " (ID: " << file_info.id << ")" << std::endl;
        process_file(file_info);
    }
    std::cout << "[Worker " << std::this_thread::get_id() << "] Exiting. Queue closed and empty." << std::endl;
}

/**
 * @brief Processes a single file: reads, tokenizes, and indexes its content.
 * @param file_info The FileInfo object containing path and ID.
 */
void IndexerWorker::process_file(const FileInfo& file_info) {
    std::ifstream file(file_info.path);
    if (!file.is_open()) {
        std::cerr << "[Worker Error] Could not open file: " << file_info.path << std::endl;
        return;
    }

    std::string line;
    size_t position_offset = 0; // Keep track of byte offset
    while (std::getline(file, line)) {
        std::vector<std::string> tokens = Utils::tokenize(line);
        size_t current_word_pos = 0; // Keep track of word index within the line
        for (const std::string& token : tokens) {
            // Note: This position calculation is simplified. For true byte offset,
            // you'd need to consider character encodings and multi-byte characters.
            // For simplicity, we'll just use a running counter for rough position.
            // Or, ideally, you could read char by char, or store byte offset from file.
            // For now, let's just add the word's position in the line as a basic example.
            index_.add_word_occurrence(token, file_info.id, position_offset + current_word_pos);
            current_word_pos += token.length() + 1; // +1 for assumed space/delimiter
        }
        position_offset += line.length() + 1; // +1 for newline character
    }
    file.close();
}