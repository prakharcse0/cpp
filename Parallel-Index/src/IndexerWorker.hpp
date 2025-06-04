// project_root/src/IndexerWorker.hpp
#pragma once

#include "ConcurrentQueue.hpp"
#include "InvertedIndex.hpp"
#include "FileInfo.hpp"
#include "Utils.hpp"
#include <fstream>    // For std::ifstream
#include <iostream>   // For std::cerr

/**
 * @brief A worker thread that processes files from a ConcurrentQueue and updates an InvertedIndex.
 *
 * This class is designed to be run as a separate thread. It continuously
 * pulls FileInfo objects from the queue, reads the file content, tokenizes it,
 * and adds word occurrences to the shared InvertedIndex.
 */
class IndexerWorker {
public:
    /**
     * @brief Constructs an IndexerWorker.
     * @param file_queue A reference to the ConcurrentQueue from which to pull files.
     * @param index A reference to the InvertedIndex to update.
     */
    IndexerWorker(ConcurrentQueue<FileInfo>& file_queue, InvertedIndex& index);

    // No copying or moving
    IndexerWorker(const IndexerWorker&) = delete;
    IndexerWorker& operator=(const IndexerWorker&) = delete;
    IndexerWorker(IndexerWorker&&) = delete;
    IndexerWorker& operator=(IndexerWorker&&) = delete;

    /**
     * @brief The main execution logic for the worker thread.
     * This method should be called by std::jthread or std::thread.
     * It continuously processes files until the queue is closed and empty.
     */
    void operator()(); // Overload operator() to make it callable by a thread

private:
    ConcurrentQueue<FileInfo>& file_queue_; ///< Reference to the queue of files to process.
    InvertedIndex& index_;                  ///< Reference to the shared inverted index.

    /**
     * @brief Processes a single file: reads, tokenizes, and indexes its content.
     * @param file_info The FileInfo object containing path and ID.
     */
    void process_file(const FileInfo& file_info);
};