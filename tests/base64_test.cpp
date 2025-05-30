#include <gtest/gtest.h>
#include <lcrypt/base64.h>

using namespace lc;

TEST(crypto, base64_b) {
    std::string a = "abcdefghijklmnopqrstuvwxyz0123456789";
    for (int i = 0; i < 10; ++i) {
        a += a;
    }
    for (int i=0; i < a.size(); ++i) {
        auto s = a.substr(0, i+1);
        EXPECT_EQ(s, base64_decode( base64_encode(s)));
    }
}

TEST(crypto, base64) {
    static const std::string in_1 = "a";
    static const std::string in_2 = "ab";
    static const std::string in_3 = "abc";
    static const std::string in_4 = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGH"
                                    "IJKLMNOPQRSTUVWXYZ0123456789";
    static const std::string in_5 = "abc123!?$*&()'-=@~";
    static const std::string in_6 = "TutorialsPoint?java8";

    static const std::string in_1_base64 = "YQ==";
    static const std::string in_2_base64 = "YWI=";
    static const std::string in_3_base64 = "YWJj";
    static const std::string in_4_base64 = "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXowMTIzNDU2Nzg5QUJDRE"
                                           "VGR0hJSktMTU5PUFFSU1RVVldYWVowMTIzNDU2Nzg5";
    static const std::string in_5_base64 = "YWJjMTIzIT8kKiYoKSctPUB+";
    static const std::string in_6_base64 = "VHV0b3JpYWxzUG9pbnQ/amF2YTg=";

    EXPECT_EQ(in_2_base64, base64_encode(in_2));
    EXPECT_EQ(in_1_base64, base64_encode(in_1));
    EXPECT_EQ(in_3_base64, base64_encode(in_3));
    EXPECT_EQ(in_4_base64, base64_encode(in_4));
    EXPECT_EQ(in_5_base64, base64_encode(in_5));
    EXPECT_EQ(in_6_base64, base64_encode(in_6));

    EXPECT_EQ(in_1, base64_decode(in_1_base64));
    EXPECT_EQ(in_2, base64_decode(in_2_base64));
    EXPECT_EQ(in_3, base64_decode(in_3_base64));
    EXPECT_EQ(in_4, base64_decode(in_4_base64));
    EXPECT_EQ(in_5, base64_decode(in_5_base64));
    EXPECT_EQ(in_6, base64_decode(in_6_base64));

    try {
        base64_decode(
            "YWJkZWZhZWRmYWZhZmFmYWZhZmFmYXNmYWZhc2Zkc2FmZGFzeHp2enZhYXNkYWRhZGFzZGFk]Q==");
        EXPECT_TRUE(false);
    } catch (const input_error& e) {
        EXPECT_EQ(e.offset(), 72);
    }
}
