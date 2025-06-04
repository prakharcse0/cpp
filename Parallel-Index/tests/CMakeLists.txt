# project_root/tests/CMakeLists.txt
# Find GTest (ensure you have GTest installed or fetch it in your root CMake)
# Example of fetching GTest in root CMakeLists.txt:
# find_package(GTest CONFIG REQUIRED)
# If GTest is not installed globally, you might need to use FetchContent in your root CMakeLists.txt
# to download and build it as part of your project.

# Ensure GTest is found in your top-level CMakeLists.txt:
# fetchcontent_declare(
#   googletest
#   GIT_REPOSITORY https://github.com/google/googletest.git
#   GIT_TAG        release-1.12.1 # Or a newer stable release
# )
# fetchcontent_makeavailable(googletest)


find_package(GTest CONFIG REQUIRED)

# Add this line to enable gtest_discover_tests
include(GoogleTest) # This command provides gtest_discover_tests

# Test for ConcurrentQueue
add_executable(test_ConcurrentQueue test_concurrent_queue.cpp)
target_link_libraries(test_ConcurrentQueue PRIVATE GTest::gtest_main ParallelIndexCore) # LINK TO LIBRARY HERE
gtest_discover_tests(test_ConcurrentQueue)

# Test for InvertedIndex
add_executable(test_InvertedIndex test_inverted_index.cpp)
target_link_libraries(test_InvertedIndex PRIVATE GTest::gtest_main ParallelIndexCore) # LINK TO LIBRARY HERE
gtest_discover_tests(test_InvertedIndex)

# Test for Utils
add_executable(test_Utils test_utils.cpp)
target_link_libraries(test_Utils PRIVATE GTest::gtest_main ParallelIndexCore) # LINK TO LIBRARY HERE
gtest_discover_tests(test_Utils)
