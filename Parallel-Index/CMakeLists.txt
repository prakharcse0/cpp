# project_root/CMakeLists.txt
# project_root/CMakeLists.txt
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(ParallelIndex LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17) # Changed from 20 to 17 for broader compatibility
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type if not specified (e.g., for IDEs)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build."
        FORCED_VALUES Debug Release MinSizeRel RelWithDebInfo)
endif()

# Enable relevant warnings for robust code
if (MSVC)
    add_compile_options(/W4 /EHsc /permissive-) # /permissive- for stricter conformance
elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror) # -Werror treats warnings as errors
endif()

# Add subdirectories for source code and tests
add_subdirectory(src)
add_subdirectory(tests) # We'll add test files later