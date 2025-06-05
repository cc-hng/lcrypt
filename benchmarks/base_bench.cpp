#include "common.h"
#include <ctime>
#include <random>
#include <hwy/contrib/algo/copy-inl.h>
#include <hwy/contrib/algo/find-inl.h>
#include <hwy/highway.h>
#include <lcrypt/base.h>

namespace hn = hwy::HWY_NAMESPACE;
static HWY_FULL(uint8_t) d8;

size_t mchr(const char* p, int ch, size_t len) {
    return hn::Find(d8, (uint8_t)ch, (const uint8_t*)p, len);
}

static int random(int m) {
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen = std::mt19937(rd());
    std::uniform_int_distribution<> dist(0, m - 1);
    return dist(gen);
}

static void bench_random(bench::Bench& b) {
    char buf1[10240] = {0};
    char buf2[10240] = {0};

    b.title("base");
    auto old = b.epochIterations();
    // b.minEpochIterations(4096000);
    b.run("rand", [&] { bench::doNotOptimizeAway(rand() % 1000); });
    b.run("random", [&] { bench::doNotOptimizeAway(random(1000)); });
    b.run("random(simd)", [&] { bench::doNotOptimizeAway(lc::random(1000)); });

    std::string s = "abcdefghijklmnopqrstuvwxyz1234567890ABC";
    b.run("strchr", [&] { bench::doNotOptimizeAway(strchr(s.c_str(), 'A')); });
    b.run("strchr(simd)", [&] { bench::doNotOptimizeAway(mchr(s.c_str(), 'A', s.size())); });
    b.run("strchr", [&] { bench::doNotOptimizeAway(memchr(buf1, 'A', 10240)); });
    b.run("strchr(simd)", [&] { bench::doNotOptimizeAway(mchr(buf1, 'A', 10240)); });
    b.run("strcpy", [&] { bench::doNotOptimizeAway(memcpy(buf2, buf1, 10240)); });
    b.run("strchr(simd)", [&] { hn::Copy(d8, (const uint8_t*)buf1, 10240, (uint8_t*)buf2); });

    b.minEpochIterations(old);
}

BENCHMARK_REGISTE(bench_random);
