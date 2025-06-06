cmake_minimum_required(VERSION 3.10)
project(TaskScheduler VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE ON) # Good for shared libraries

# Include directories
include_directories(include)

# Source files
set(TASK_SCHEDULER_SRCS
    src/task.cpp
    src/task_scheduler.cpp
    src/thread_pool.cpp
    src/priority_queue.cpp
    src/dependency_manager.cpp
)

# Create a static library (or shared, depending on your needs)
add_library(TaskScheduler STATIC ${TASK_SCHEDULER_SRCS})

# Add tests
enable_testing()
add_subdirectory(tests)

# Add examples
add_subdirectory(examples)

# Add benchmarks
add_subdirectory(benchmarks)

# Optional: Install rules (if you want to install the library)
# install(TARGETS TaskScheduler DESTINATION lib)
# install(DIRECTORY include/scheduler DESTINATION include)