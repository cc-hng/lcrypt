#include "common.h"
#include <ctime>
#include <exception>
#include <random>
#include <stdexcept>

#if 0
#    include <hwy/contrib/random/random-inl.h>

inline uint64_t GetSeed() {
    return static_cast<uint64_t>(std::time(nullptr));
}

static int random_simd(int m) {
    static hwy::HWY_NAMESPACE::CachedXoshiro<> generator{GetSeed()};
    return generator();
}

#else

static int random_simd(int m) {
    return 0;
}

#endif

static int random(int m) {
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen = std::mt19937(rd());
    std::uniform_int_distribution<> dist(0, m - 1);
    return dist(gen);
}

static void bench_random(bench::Bench& b) {
    b.title("random");
    auto old = b.epochIterations();
    b.minEpochIterations(4096000);
    b.run("random", [&] { bench::doNotOptimizeAway(random(1000)); });
    b.run("random(simd)", [&] { bench::doNotOptimizeAway(random_simd(1000)); });
    b.minEpochIterations(old);
}

BENCHMARK_REGISTE(bench_random);
