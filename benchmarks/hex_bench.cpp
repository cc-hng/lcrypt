#include "common.h"
#include <exception>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <lcrypt/hex.h>

using namespace lc;

std::string hex_marshal(std::string_view what) {
    static char hex[]   = "0123456789abcdef";
    size_t sz           = what.size();
    const uint8_t* text = (const uint8_t*)(what.data());
    std::string out(sz * 2, '\0');
    char* buffer = &out[0];
    int i;
    for (i = 0; i < (int)sz; i++) {
        buffer[i * 2]     = hex[text[i] >> 4];
        buffer[i * 2 + 1] = hex[text[i] & 0xf];
    }
    return out;
}

#define HEX(v, c)                              \
    {                                          \
        char tmp = (char)c;                    \
        if (tmp >= '0' && tmp <= '9') {        \
            v = tmp - '0';                     \
        } else if (tmp >= 'A' && tmp <= 'F') { \
            v = tmp - 'A' + 10;                \
        } else {                               \
            v = tmp - 'a' + 10;                \
        }                                      \
    }

std::string hex_unmarshal(std::string_view what) {
    size_t sz        = what.size();
    const char* text = what.data();
    if (sz & 1) {
        throw std::runtime_error("Invalid hex text size");
    }
    std::string out(sz / 2, '\0');
    char* buffer = &out[0];
    int i;
    for (i = 0; i < (int)sz; i += 2) {
        uint8_t hi, low;
        HEX(hi, text[i]);
        HEX(low, text[i + 1]);
        if (hi > 16 || low > 16) {
            fprintf(stderr, "hi:%d, lo:%d, %c\n", hi, low, text[i + 1]);
            throw std::runtime_error("Invalid hex text");
        }
        buffer[i / 2] = hi << 4 | low;
    }
    return out;
}

static const std::string input =
    "hello,"
    "world111111111111111111111111111111111111111111111111111111111111111111111111111111111111"
    "11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111"
    "11111111111111111111111111111111111111111111111111111111111111111111111111112222222222222"
    "222222222222222222222222222"
    "333333333333333333333333333333333333333333333333333333333333333"
    "444444444444444444444444444444444444444444444444444444444444444"
    "555555555555555555555555555555555555555555555555555555555555555"
    "666666666666666666666666666666666666666666666666666666666666666"
    "777777777777777777777777777777777777777777777777777777777777777"
    "888888888888888888888888888888888888888888888888888888888888888"
    "999999999999999999999999999999999999999999999999999999999999999"
    "000000000000000000000000000000000000000000000000000000000000000"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaaaaaaaaaaaa"
    "aaaaaaaaaaa"
    "aaaa";
static std::string input_hex = hex_encode(input);

static void bench_hex(bench::Bench& b) {
    b.title("hex");
    auto old = b.epochIterations();
    b.minEpochIterations(40960);

    b.run("hex::encode(simd)", [&] { bench::doNotOptimizeAway(hex_encode(input)); });
    b.run("hex::encode", [&] { bench::doNotOptimizeAway(hex_marshal(input)); });
    b.run("hex::decode(simd)", [&] { bench::doNotOptimizeAway(hex_decode(input_hex)); });
    b.run("hex::decode", [&] { bench::doNotOptimizeAway(hex_unmarshal(input_hex)); });

    b.minEpochIterations(old);
}
BENCHMARK_REGISTE(bench_hex);
