
#include <gtest/gtest.h>
#include <lcrypt/base.h>

TEST(crypto, random) {
    static constexpr int times = 5e6;

    for (int i = 0; i < times; i++) {
        auto r = lc::random(100, 200);
        EXPECT_TRUE(r >= 100 && r <= 200);
    }

    for (int i = 0; i < times; i++) {
        auto r = lc::random(100);
        EXPECT_TRUE(r >= 0 && r < 100);
    }

    for (int i = 0; i < times; i++) {
        auto r = lc::random();
        EXPECT_TRUE(r >= 0 && r < 1.0);
    }
}
