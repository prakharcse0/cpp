#include "FileScanner.hpp"
#include "IndexerWorker.hpp"
#include "InvertedIndex.hpp"
#include "ConcurrentQueue.hpp"
#include "FileInfo.hpp" // Required for FileInfo type
#include "Utils.hpp"    // Required for Utils::to_lower

#include <memory>        // For std::unique_ptr
#include <iostream>      // For console input/output
#include <vector>        // For std::vector
#include <thread>        // Explicitly using std::thread for multi-threading
#include <string>        // For std::string
#include <chrono>        // For measuring time (high_resolution_clock)
#include <set>           // For std::set (used for unique file IDs in search results, and stop words)
#include <unordered_map> // For std::unordered_map (used for g_file_id_to_path_map)
#include <stdexcept>     // For std::out_of_range

// A global map to store file ID to path mapping for search results.
std::unordered_map<size_t, std::string> g_file_id_to_path_map;

int main(int argc, char* argv[]) {
    std::cout << "--- ParallelIndex: Concurrent File Indexer & Search Engine ---" << std::endl;

    // Command-line argument parsing
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <directory_to_index> [num_indexer_threads]" << std::endl;
        std::cerr << "Example: " << argv[0] << " ./my_documents 8" << std::endl;
        return 1;
    }

    std::string root_dir_str = argv[1];
    int num_indexer_threads = 4; // Default number of worker threads

    if (argc >= 3) {
        try {
            num_indexer_threads = std::stoi(argv[2]);
            if (num_indexer_threads <= 0) {
                throw std::out_of_range("Number of threads must be positive.");
            }
        } catch (const std::exception& e) {
            std::cerr << "Warning: Invalid number of threads provided (" << argv[2] << "). Error: " << e.what() << ". Using default " << num_indexer_threads << " threads." << std::endl;
        }
    }

    // Set some common stop words to be ignored during tokenization
    // Utils::set_stop_words({"a", "an", "the", "and", "or", "but", "is", "are", "was", "were", "of", "in", "to", "for", "on", "with", "as", "at", "it", "its", "by"});
    Utils::set_stop_words({});

    // Instantiate core components
    ConcurrentQueue<FileInfo> file_queue;
    InvertedIndex inverted_index;

    // --- Indexing Phase ---
    auto start_time = std::chrono::high_resolution_clock::now();

    // 1. Create and start the FileScanner thread
    // The scanner will discover files and push them into 'file_queue'.
    // std::thread is used, so we must explicitly call .join() later.
    FileScanner scanner(root_dir_str, file_queue);
    std::thread scanner_thread([&scanner]{ scanner.start_scanning(); });

    // 2. Create and start multiple IndexerWorker threads
    // Each worker will pull files from 'file_queue' and update 'inverted_index'.
    // We use unique_ptr to manage the lifetime of IndexerWorker objects as they are non-copyable/movable.
    std::vector<std::unique_ptr<IndexerWorker>> worker_objects;
    std::vector<std::thread> worker_threads;

    for (int i = 0; i < num_indexer_threads; ++i) {
        // Create a unique IndexerWorker object
        auto worker = std::make_unique<IndexerWorker>(file_queue, inverted_index);

        // Store the unique_ptr in the vector. std::vector::push_back can take a movable unique_ptr.
        worker_objects.push_back(std::move(worker));

        // Start a thread. The lambda captures a raw pointer to the worker object.
        // This is safe because worker_objects ensures the lifetime of the object,
        // and all threads will be joined before worker_objects goes out of scope.
        worker_threads.emplace_back([worker_ptr = worker_objects.back().get()]() {
            (*worker_ptr)(); // Call the operator() of the IndexerWorker object
        });
    }

    std::cout << "Indexing started for directory: " << root_dir_str << " with " << num_indexer_threads << " worker threads." << std::endl;

    // Wait for the scanner thread to finish its work and close the file queue.
    scanner_thread.join();

    // Wait for all worker threads to finish. They will naturally exit their loops
    // once the file queue is closed AND becomes empty.
    for (auto& t : worker_threads) {
        t.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    std::cout << "\n--- Indexing Complete ---" << std::endl;
    std::cout << "Total unique words indexed: " << inverted_index.size() << std::endl;
    std::cout << "Indexing time: " << duration_ms.count() << " ms" << std::endl;

    // --- Search Phase ---
    std::string query;
    std::cout << "\n--- Search Mode ---" << std::endl;
    std::cout << "Enter 'q' or 'quit' to exit." << std::endl;

    while (true) {
        std::cout << "\nEnter word to search: ";
        std::getline(std::cin, query); // Use getline to handle spaces in query (though we only tokenize single words)

        if (query == "q" || query == "quit") {
            break;
        }
        if (query.empty()) {
            continue;
        }

        std::string processed_query = Utils::to_lower(query); // Convert query to lowercase for consistent search

        auto search_start_time = std::chrono::high_resolution_clock::now();
        std::vector<WordOccurrence> results = inverted_index.search(processed_query);
        auto search_end_time = std::chrono::high_resolution_clock::now();
        auto search_duration_us = std::chrono::duration_cast<std::chrono::microseconds>(search_end_time - search_start_time);

        if (results.empty()) {
            std::cout << "No matches found for '" << query << "'." << std::endl;
        } else {
            // Collect unique file IDs to display paths only once per file
            std::set<size_t> matched_file_ids;
            for(const auto& occ : results) {
                matched_file_ids.insert(occ.file_id);
            }

            std::cout << "Found '" << query << "' in " << matched_file_ids.size() << " unique files (" << results.size() << " occurrences total):" << std::endl;
            for (size_t file_id : matched_file_ids) {
                std::cout << "- File ID: " << file_id << std::endl;
                // Example of how you'd use g_file_id_to_path_map if populated:
                // if (g_file_id_to_path_map.count(file_id)) {
                //     std::cout << "- File: " << g_file_id_to_path_map[file_id] << std::endl;
                // } else {
                //     std::cout << "- File ID: " << file_id << " (Path not available)" << std::endl;
                // }
            }
        }
        std::cout << "Search time: " << search_duration_us.count() << " us" << std::endl;
    }

    std::cout << "Exiting ParallelIndex. Goodbye!" << std::endl;
    return 0;
}