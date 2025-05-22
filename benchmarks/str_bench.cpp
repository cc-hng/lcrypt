#include "common.h"
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <lcrypt/str.h>
#include <string.h>

inline std::string str_toupper0(std::string_view str) {
    std::string result(str);
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

inline std::string str_tolower0(std::string_view str) {
    std::string result(str);
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

static const std::string input =
    "hello111111111111111111111111111111111111111111111111111111111111111111111111,"
    "world111111111111111111111111111111111111111111111111111111111111111111111111111111111111,"
    "11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111,"
    "11111111111111111111111111111111111111111111111111111111111111111111111111112222222222222,"
    "222222222222222222222222222,"
    "333333333333333333333333333333333333333333333333333333333333333,"
    "444444444444444444444444444444444444444444444444444444444444444,"
    "555555555555555555555555555555555555555555555555555555555555555,"
    "666666666666666666666666666666666666666666666666666666666666666,"
    "777777777777777777777777777777777777777777777777777777777777777,"
    "888888888888888888888888888888888888888888888888888888888888888,"
    "999999999999999999999999999999999999999999999999999999999999999,"
    "000000000000000000000000000000000000000000000000000000000000000,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA,"
    "333333333333333333333333333333333333333333333333333333333333333,"
    "444444444444444444444444444444444444444444444444444444444444444,"
    "555555555555555555555555555555555555555555555555555555555555555,"
    "666666666666666666666666666666666666666666666666666666666666666,"
    "777777777777777777777777777777777777777777777777777777777777777,"
    "888888888888888888888888888888888888888888888888888888888888888,"
    "999999999999999999999999999999999999999999999999999999999999999,"
    "000000000000000000000000000000000000000000000000000000000000000,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA,"
    "aaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaaaaaaaaaaaa,"
    "aaaaaaaaaaa,"
    "aaaa";

static void bench_string(bench::Bench& b) {
    auto input_sv = std::string_view(input);
    auto splitted = lc::str_split(input, ",");
    b.title("string");
    auto old = b.epochIterations();
    b.minEpochIterations(10240);
    b.run("toupper", [&] { bench::doNotOptimizeAway(str_toupper0(input)); });
    b.run("toupper(simd)", [&] { bench::doNotOptimizeAway(lc::str_toupper(input)); });
    b.run("toupper-small",
          [&] { bench::doNotOptimizeAway(str_toupper0("aabcdefghijklabcdefghijklbcdefghi")); });
    b.run("toupper-small(simd)",
          [&] { bench::doNotOptimizeAway(lc::str_toupper("aabcdefghijklabcdefghijklbcdefghi")); });
    b.run("tolower", [&] { bench::doNotOptimizeAway(str_tolower0(input)); });
    b.run("tolower(simd)", [&] { bench::doNotOptimizeAway(lc::str_tolower(input)); });
    b.run("split", [&] { bench::doNotOptimizeAway(lc::str_split(input, ",")); });
    b.run("join", [&] { bench::doNotOptimizeAway(lc::str_join(splitted, ",")); });

    b.minEpochIterations(old);
}

BENCHMARK_REGISTE(bench_string);
