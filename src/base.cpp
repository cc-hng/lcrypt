#include "lcrypt/base.h"
#include <hwy/contrib/random/random-inl.h>
#include <time.h>

namespace {

inline uint64_t GetSeed() {
    return static_cast<uint64_t>(time(nullptr));
}

inline uint64_t lrandom() {
    static hwy::HWY_NAMESPACE::CachedXoshiro<> generator{GetSeed()};
    return generator();
}

}  // namespace

namespace lc {

double random() {
    double r = (double)lrandom() * (1.0 / ((double)hwy::LimitsMax<uint64_t>() + 1.0));
    return r;
}

size_t random(size_t m) {
    return random(0, m - 1);
}

size_t random(size_t m, size_t n) {
    double r = random();
    r *= (double)(n - m) + 1.0;
    return m + (size_t)r;
}

}  // namespace lc
