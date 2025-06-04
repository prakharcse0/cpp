// project_root/src/FileScanner.hpp
#pragma once

#include "ConcurrentQueue.hpp"
#include "FileInfo.hpp"
#include <filesystem> // C++17
#include <atomic>     // For std::atomic
#include <string>

/**
 * @brief Scans a specified root directory and enqueues files for indexing.
 *
 * This class traverses the file system using std::filesystem and pushes
 * eligible files (e.g., text files) into a ConcurrentQueue for processing
 * by indexer worker threads.
 */
class FileScanner {
public:
    /**
     * @brief Constructs a FileScanner.
     * @param root_dir The starting directory to scan.
     * @param file_queue A reference to the ConcurrentQueue to push FileInfo objects into.
     */
    FileScanner(const std::string& root_dir, ConcurrentQueue<FileInfo>& file_queue);

    // No copying or moving
    FileScanner(const FileScanner&) = delete;
    FileScanner& operator=(const FileScanner&) = delete;
    FileScanner(FileScanner&&) = delete;
    FileScanner& operator=(FileScanner&&) = delete;

    /**
     * @brief Starts the file scanning process.
     * This method is designed to be run in its own thread.
     * It will traverse the directory and close the queue when finished.
     */
    void start_scanning();

private:
    std::filesystem::path root_directory_; ///< The path to the root directory to scan.
    ConcurrentQueue<FileInfo>& file_queue_; ///< Reference to the queue for discovered files.
    std::atomic<size_t> next_file_id_ = 0; ///< Atomically increments for unique file IDs.

    /**
     * @brief Recursively scans a directory and its subdirectories.
     * @param current_path The current path being scanned.
     */
    void scan_directory(const std::filesystem::path& current_path);

    /**
     * @brief Checks if a file should be indexed (e.g., based on extension).
     * @param path The path to the file.
     * @return True if the file should be indexed, false otherwise.
     */
    bool is_indexable_file(const std::filesystem::path& path) const;
};