#include "FileScanner.hpp"
#include <iostream> // For logging errors/info
#include <set>      // For indexable extensions
#include <algorithm>

// Define a set of indexable file extensions
static const std::set<std::string> indexable_extensions = {
    ".txt", ".md", ".cpp", ".hpp", ".c", ".h", ".json", ".xml", ".log"
    // Add more as needed
};

/**
 * @brief Constructs a FileScanner.
 * @param root_dir The starting directory to scan.
 * @param file_queue A reference to the ConcurrentQueue to push FileInfo objects into.
 */
FileScanner::FileScanner(const std::string& root_dir, ConcurrentQueue<FileInfo>& file_queue)
    : root_directory_(root_dir), file_queue_(file_queue) {
    if (!std::filesystem::exists(root_directory_)) {
        throw std::filesystem::filesystem_error(
            "Root directory does not exist", root_directory_,
            std::make_error_code(std::errc::no_such_file_or_directory)
        );
    }
    if (!std::filesystem::is_directory(root_directory_)) {
        throw std::filesystem::filesystem_error(
            "Path is not a directory", root_directory_,
            std::make_error_code(std::errc::not_a_directory)
        );
    }
}

/**
 * @brief Starts the file scanning process.
 * This method is designed to be run in its own thread.
 * It will traverse the directory and close the queue when finished.
 */
void FileScanner::start_scanning() {
    try {
        std::cout << "[Scanner] Starting scan of: " << root_directory_ << std::endl;
        scan_directory(root_directory_);
        std::cout << "[Scanner] Scan complete. Total files enqueued: " << next_file_id_ << std::endl;
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "[Scanner Error] Filesystem error during scan: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Scanner Error] An unexpected error occurred during scan: " << e.what() << std::endl;
    }
    file_queue_.close(); // Signal to consumers that no more files will be added
    std::cout << "[Scanner] File queue closed." << std::endl;
}

/**
 * @brief Recursively scans a directory and its subdirectories.
 * @param current_path The current path being scanned.
 */
void FileScanner::scan_directory(const std::filesystem::path& current_path) {
    // std::filesystem::recursive_directory_iterator handles recursion automatically
    for (const auto& entry : std::filesystem::recursive_directory_iterator(current_path, std::filesystem::directory_options::skip_permission_denied)) {
        try {
            if (entry.is_regular_file()) {
                if (is_indexable_file(entry.path())) {
                    size_t file_id = next_file_id_.fetch_add(1); // Atomically get and increment
                    file_queue_.push(FileInfo(file_id, entry.path().string()));
                    // std::cout << "[Scanner] Enqueued file: " << entry.path().filename() << " (ID: " << file_id << ")" << std::endl;
                }
            }
        } catch (const std::filesystem::filesystem_error& e) {
            std::cerr << "[Scanner Warning] Skipping " << entry.path() << " due to: " << e.what() << std::endl;
        }
    }
}

/**
 * @brief Checks if a file should be indexed (e.g., based on extension).
 * @param path The path to the file.
 * @return True if the file should be indexed, false otherwise.
 */
bool FileScanner::is_indexable_file(const std::filesystem::path& path) const {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return indexable_extensions.count(extension) > 0;
}