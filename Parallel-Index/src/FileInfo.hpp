#pragma once

#include <string>

//  Represents metadata for a file to be indexed.

struct FileInfo {
    size_t id;         ///< A unique identifier for the file within the index.
    std::string path;  ///< The full absolute path to the file.

    // Default constructor for convenience
    FileInfo() : id(0), path("") {}
    FileInfo(size_t id, std::string path) : id(id), path(std::move(path)) {}
};