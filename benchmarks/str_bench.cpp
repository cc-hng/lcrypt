#include "common.h"
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <lcrypt/str.h>
#include <string.h>

// 字符串分割函数
// inline std::vector<std::string_view> str_split0(std::string_view str, std::string_view delimiter) {
//     std::vector<std::string_view> result;
//     size_t start = 0;
//     size_t end   = 0;
//
//     while ((end = str.find(delimiter, start)) != std::string_view::npos) {
//         result.emplace_back(str.substr(start, end - start));  // 提取子字符串
//         start = end + delimiter.size();                       // 移动到下一个位置
//     }
//     result.emplace_back(str.substr(start));  // 添加最后部分
//     return result;
// }

inline std::vector<std::string_view>  //
str_split0(std::string_view str, std::string_view delimiter) {
    size_t start       = 0;
    size_t end         = 0;
    size_t dlen        = delimiter.size();
    size_t slen        = str.size();
    const char* ps     = str.data();
    const char* ps_end = ps + slen;
    const char* pd     = delimiter.data();
    std::vector<std::string_view> result;
    result.reserve(16);

    for (;;) {
        auto p = (const char*)memmem(ps, ps_end - ps, pd, dlen);
        if (!p) {
            result.emplace_back(str.substr(start));
            break;
        }
        result.emplace_back(std::string_view(ps, p));
        ps = p + 1;
    }

    return result;
}

std::string str_join0(const std::vector<std::string_view>& vs, std::string_view delimiter) {
    int count      = 0;
    size_t dlen    = delimiter.size();
    const char* pd = delimiter.data();
    for (const auto& s : vs) {
        count += s.size();
    }
    count += dlen * (vs.size() - 1);

    std::string out;
    out.reserve(count);
    bool first = true;
    for (const auto& s : vs) {
        if (!first) {
            out.append(delimiter);
        }
        out.append(s);
    }
    return out;
}

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
    // b.minEpochIterations(4096000);
    b.run("toupper(simd)", [&] { bench::doNotOptimizeAway(lc::str_toupper(input)); });
    b.run("toupper", [&] { bench::doNotOptimizeAway(str_toupper0(input)); });
    b.run("tolower(simd)", [&] { bench::doNotOptimizeAway(lc::str_tolower(input)); });
    b.run("tolower", [&] { bench::doNotOptimizeAway(str_tolower0(input)); });
    b.run("split1", [&] { bench::doNotOptimizeAway(lc::str_split(input, ",")); });
    b.run("split2", [&] { bench::doNotOptimizeAway(str_split0(input, ",")); });
    b.run("join1", [&] { bench::doNotOptimizeAway(lc::str_join(splitted, ",")); });
    b.run("join2", [&] { bench::doNotOptimizeAway(str_join0(splitted, ",")); });

    b.minEpochIterations(old);
}

BENCHMARK_REGISTE(bench_string);
