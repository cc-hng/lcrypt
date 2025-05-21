#include <gtest/gtest.h>
#include <lcrypt/hex.h>
#include <lcrypt/str.h>

using namespace lc;
using str_t = std::string;

template <typename T, std::size_t... I>
auto slice_tuple_impl(std::index_sequence<I...>, T&& tuple) {
    return std::make_tuple(std::get<I>(std::forward<decltype(tuple)>(tuple))...);
}

template <typename... Ts>
inline auto tuple_pop_back(const std::tuple<Ts...>& t) {
    constexpr std::size_t N = sizeof...(Ts);
    static_assert(N > 0, "tuple must have at least one element");
    return slice_tuple_impl(std::make_index_sequence<N - 1>{}, t);
}

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

TEST(crypto, pack) {
    EXPECT_EQ(hex_encode(str_pack("i2", 1)), "0100");

    EXPECT_EQ(tuple_pop_back(str_unpack<uint8_t>("B", str_pack("B", 0xff))),
              std::make_tuple<uint8_t>(0xff));
    EXPECT_EQ(tuple_pop_back(str_unpack<int8_t>("b", str_pack("b", 0x7f))),
              std::make_tuple<int8_t>(0x7f));
    EXPECT_EQ(tuple_pop_back(str_unpack<int8_t>("b", str_pack("b", -0x80))),
              std::make_tuple<int8_t>(-0x80));

    EXPECT_EQ(tuple_pop_back(str_unpack<uint16_t>("H", str_pack("H", 0xffff))),
              std::make_tuple<uint16_t>(0xffff));
    EXPECT_EQ(tuple_pop_back(str_unpack<int16_t>("h", str_pack("h", 0x7fff))),
              std::make_tuple<int16_t>(0x7fff));
    EXPECT_EQ(tuple_pop_back(str_unpack<int16_t>("h", str_pack("h", -0x8000))),
              std::make_tuple<int16_t>(-0x8000));

    EXPECT_EQ(tuple_pop_back(str_unpack<uint32_t>("L", str_pack("L", 0xffffffff))),
              std::make_tuple<uint32_t>(0xffffffff));
    EXPECT_EQ(tuple_pop_back(str_unpack<int32_t>("l", str_pack("l", 0x7fffffff))),
              std::make_tuple<int32_t>(0x7fffffff));
    EXPECT_EQ(tuple_pop_back(str_unpack<int32_t>("l", str_pack("l", -0x80000000))),
              std::make_tuple<int32_t>(-0x80000000));

    auto r = str_pack("zB", "abc", 247);
    EXPECT_EQ(hex_encode(r), "61626300f7");
    EXPECT_EQ((str_unpack<std::string, int>("zB", r)),
              (std::make_tuple<std::string, int, int>("abc", 247, r.size())));

    EXPECT_EQ((to_span(str_pack("<! c3", "abc"))), "abc");
    EXPECT_EQ((to_span(str_pack("<!4 c6", "abcdef"))), "abcdef");
    EXPECT_EQ(hex_encode(to_span(str_pack("c8", "123456"))), "3132333435360000");

    do {
        char buf[] = "abcdefghi\0xyz";
        auto [a, b, pos] =
            str_unpack<std::string, std::string>("!4 z c3", std::string_view(buf, 13));
        EXPECT_EQ(a, "abcdefghi");
        EXPECT_EQ(b, "xyz");
        EXPECT_EQ(pos, 13);
    } while (0);

    do {
        auto r = str_pack("<b h b f d f I i", 1, 2, 3, 4, 5, 6, 7, 8);
        auto [a, b, c, d, e, f, g, h, pos] =
            str_unpack<int, int, int, int, int, int, int, int>("<b h b f d f I i", r);
        EXPECT_TRUE(a == 1 && b == 2 && c == 3 && d == 4 && e == 5 && f == 6 && g == 7 && h == 8);
        EXPECT_EQ(pos, r.size());
    } while (0);

    EXPECT_EQ(hex_encode(str_pack(" < i1 i2 ", 2, 3)), "020300");

    do {
        auto r = str_pack(" >!8 b Xh i4 i8 c1 Xi8", -12, 100, 200, "\xEC");
        EXPECT_EQ(hex_encode(r), "f40000000000006400000000000000c8ec00000000000000");
        auto [a, b, c, d, pos] = str_unpack<int, int, int, std::string>(" >!8 b Xh i4 i8 c1 Xi8", r);
        EXPECT_EQ(a, -12);
        EXPECT_EQ(b, 100);
        EXPECT_EQ(c, 200);
        EXPECT_EQ(d, "\xEC");
        EXPECT_EQ(pos, r.size());
    } while (0);

    do {
        std::string fmt = ">!4 c3 c4 c2 z i4 c5 c2 Xi4";
        auto r          = str_pack(fmt, "abc", "abcd", "xz", "hello", 5, "world", "xy");
        EXPECT_EQ(hex_encode(r), "61626361626364787a68656c6c6f000000000005776f726c64787900");
        auto [a, b, c, d, e, f, g, pos] =
            str_unpack<str_t, str_t, str_t, str_t, int, str_t, str_t>(fmt, r);
        EXPECT_EQ(a, "abc");
        EXPECT_EQ(b, "abcd");
        EXPECT_EQ(c, "xz");
        EXPECT_EQ(d, "hello");
        EXPECT_EQ(e, 5);
        EXPECT_EQ(f, "world");
        EXPECT_EQ(g, "xy");
        EXPECT_EQ(pos, r.size());
    } while (0);

    do {
        str_t fmt = " b b Xd b Xb x";
        auto r    = str_pack(fmt, 1, 2, 3);
        EXPECT_EQ(hex_encode(r), "01020300");
        auto [a, b, c, pos] = str_unpack<int, int, int>(fmt, r);
        EXPECT_EQ(a, 1);
        EXPECT_EQ(b, 2);
        EXPECT_EQ(c, 3);
        EXPECT_EQ(pos, r.size());
    } while (0);
}
