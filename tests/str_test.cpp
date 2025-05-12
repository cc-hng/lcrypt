#include <gtest/gtest.h>
#include <lcrypt/str.h>

using namespace lc;

TEST(crypto, string) {
    std::string s1 = "abcABC123.-+/";

    // toupper | tolower
    EXPECT_EQ(str_toupper(s1), "ABCABC123.-+/");
    EXPECT_EQ(str_tolower(s1), "abcabc123.-+/");

    // join | split
    std::string s2                          = "a,,bb,,ccc,,dddd";
    std::vector<std::string_view> splitted1 = {"a", "", "bb", "", "ccc", "", "dddd"};
    std::vector<std::string_view> splitted2 = {"a", "bb", "ccc", "dddd"};
    EXPECT_EQ(str_split(s2, ","), splitted1);
    EXPECT_EQ(str_split(s2, ",,"), splitted2);

    EXPECT_EQ(str_join(splitted2, ","), "a,bb,ccc,dddd");
    EXPECT_EQ(str_join(splitted2, ",,"), "a,,bb,,ccc,,dddd");

    // trim
    EXPECT_EQ(str_trim(" abc "), "abc");

    // starts_with | ends_with
    EXPECT_TRUE(str_starts_with("abc:test", "abc"));
    EXPECT_TRUE(str_ends_with("abc.txt", ".txt"));
}
