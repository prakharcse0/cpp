// project_root/tests/test_utils.cpp
#include <gtest/gtest.h>
#include "Utils.hpp"
#include <set>

TEST(UtilsTest, ToLower) {
    EXPECT_EQ(Utils::to_lower("Hello World"), "hello world");
    EXPECT_EQ(Utils::to_lower("C++ PROGRAMMING"), "c++ programming");
    EXPECT_EQ(Utils::to_lower("123ABCxyz"), "123abcxyz");
    EXPECT_EQ(Utils::to_lower(""), "");
}

TEST(UtilsTest, TokenizeBasic) {
    std::vector<std::string> tokens = Utils::tokenize("Hello, world! This is a test.");
    ASSERT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens[0], "hello");
    EXPECT_EQ(tokens[1], "world");
    EXPECT_EQ(tokens[2], "this");
    EXPECT_EQ(tokens[3], "is");
    EXPECT_EQ(tokens[4], "a");
    EXPECT_EQ(tokens[5], "test");
}

TEST(UtilsTest, TokenizeWithNumbersAndSpecialChars) {
    std::vector<std::string> tokens = Utils::tokenize("C++ is fun with 2023 features!");
    ASSERT_EQ(tokens.size(), 6);
    EXPECT_EQ(tokens[0], "c"); // C++ is tricky due to ++ being removed
    EXPECT_EQ(tokens[1], "is");
    EXPECT_EQ(tokens[2], "fun");
    EXPECT_EQ(tokens[3], "with");
    EXPECT_EQ(tokens[4], "2023");
    EXPECT_EQ(tokens[5], "features");
}

TEST(UtilsTest, TokenizeEmptyString) {
    std::vector<std::string> tokens = Utils::tokenize("");
    EXPECT_TRUE(tokens.empty());
}

TEST(UtilsTest, TokenizeWhitespaceOnly) {
    std::vector<std::string> tokens = Utils::tokenize("   \t \n ");
    EXPECT_TRUE(tokens.empty());
}

TEST(UtilsTest, StopWords) {
    Utils::set_stop_words({"is", "the", "a"});
    std::vector<std::string> tokens = Utils::tokenize("This is a test of the stop words.");
    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0], "this");
    EXPECT_EQ(tokens[1], "test");
    EXPECT_EQ(tokens[2], "of");
    EXPECT_EQ(tokens[3], "stop");
    EXPECT_EQ(tokens[4], "words");
}