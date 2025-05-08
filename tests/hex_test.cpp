#include <gtest/gtest.h>
#include <lcrypt/hex.h>

using namespace lc;

TEST(crypto, hex) {
    static const std::string input_1 = "hello,world";  // not simd
    static const std::string input_2 = "abcdefghijklmnopqrstuvwxyz0123456789ABCDE"
                                       "FGHIJKLMNOPQRSTUVWXYZ0123456789";  // simd
    static const std::string input_3 =
        "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";  // simd
    static const std::string input_1_hex = "68656c6c6f2c776f726c64";
    static const std::string input_2_hex = "6162636465666768696a6b6c6d6e6f707172737475767778797a30"
                                           "3132333435363738394142434445464748494a4"
                                           "b4c4d4e4f505152535455565758595a30313233343536373839";
    static const std::string input_3_hex = "6162636465666768696a6b6c6d6e6f707172737475767778797a30"
                                           "3132333435363738394142434445464748494a4"
                                           "b4c4d4e4f505152535455565758595a";

    // encode - decode
    EXPECT_EQ(input_2_hex, hex_encode(input_2));
    EXPECT_EQ(input_2, hex_decode(input_2_hex));

    EXPECT_EQ(input_1_hex, hex_encode(input_1));
    EXPECT_EQ(input_1, hex_decode(input_1_hex));

    EXPECT_EQ(input_3_hex, hex_encode(input_3));
    EXPECT_EQ(input_3, hex_decode(input_3_hex));

    // is_hex
    // EXPECT_TRUE(cc::hex::is_hex("012345"));
    // EXPECT_TRUE(!cc::hex::is_hex("12345"));
    // EXPECT_TRUE(!cc::hex::is_hex("12345G"));
}
