# project_root/src/CMakeLists.txt
add_library(ParallelIndexCore STATIC # Create a static library named ParallelIndexCore
    FileScanner.cpp
    IndexerWorker.cpp
    InvertedIndex.cpp
    Utils.cpp
)

# Set include directories for the library
target_include_directories(ParallelIndexCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Now, add the main executable which links against this library
add_executable(ParallelIndex main.cpp)
target_link_libraries(ParallelIndex PRIVATE ParallelIndexCore)