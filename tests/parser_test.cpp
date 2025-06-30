#include "parser.hpp"
#include <gtest/gtest.h>

TEST(ParserTest, CharacterParser)
{
    constexpr auto cp1 = characterParser('a').parse("australia");
    ASSERT_TRUE(cp1.is_ok());
    constexpr auto cp2 = characterParser('p').parse("australia");
    ASSERT_TRUE(cp2.is_err());
    constexpr auto cp3 = characterParser('s').parse("");
    ASSERT_TRUE(cp2.is_err());
}

TEST(ParserTest, StringParser)
{
    constexpr auto sp1 = stringParser("C++").parse("C++ is amazing!");
    ASSERT_TRUE(sp1.is_ok());
    ASSERT_EQ(sp1.unwrap(), "C++");
    ASSERT_EQ(sp1.index(),
        3); // After parsing index moves to space literal after C++
    constexpr auto sp2 = stringParser("Java").parse("C++ is amazing");
    ASSERT_TRUE(
        sp2.is_err()); // Java doesn't match C++ bcoz it's not amazing
    constexpr auto sp3 = stringParser("JS").parse("");
    ASSERT_TRUE(sp3.is_err());
    constexpr auto sp4
        = stringParser("").parse("Python"); // empty can be matched
    ASSERT_TRUE(sp4.is_ok());
}

TEST(ParserTest, ChoiceParser)
{
    // constexpr auto cp1 = choice(stringParser("hello"),
    // stringParser("world"));
}
