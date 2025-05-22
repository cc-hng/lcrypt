#include "lcrypt/base.h"
#include <hwy/contrib/random/random-inl.h>
#include <time.h>

namespace hn = hwy::HWY_NAMESPACE;

namespace {

inline uint64_t GetSeed() {
    return static_cast<uint64_t>(time(nullptr));
}

template <std::uint64_t size = 1024>
class FloatCachedXoshiro {
public:
    using result_type = double;
    enum { N = hn::Lanes(hn::ScalableTag<uint64_t>()) };

    explicit FloatCachedXoshiro(const result_type seed, const result_type threadNumber = 0)
      : generator_{seed, threadNumber}
      , cache_{generator_.Uniform<size>()}
      , index_{0} {}

    result_type operator()() noexcept {
        if (HWY_UNLIKELY(index_ == size)) {
            cache_ = std::move(generator_.Uniform<size>());
            index_ = 0;
        }
        return cache_[index_++];
    }

private:
    hn::VectorXoshiro generator_;
    std::array<result_type, size> cache_;
    std::size_t index_;

    static_assert((size & (size - 1)) == 0 && size != 0, "only power of 2 are supported");
};

inline double random_impl() {
#if HWY_HAVE_FLOAT64
    static FloatCachedXoshiro<> generator{GetSeed()};
    return generator();
#else
    static hn::CachedXoshiro<64> generator{GetSeed()};
    static constexpr uint64_t max = hwy::LimitsMax<uint64_t>();
    return ((double)generator() * (1.0 / ((double)max + 1.0)));
#endif
}

inline size_t random_impl(size_t m, size_t n) {
    double r = random_impl();
    r *= (double)(n - m) + 1.0;
    return m + (size_t)r;
}

}  // namespace

namespace lc {

double random() {
    return random_impl();
}

size_t random(size_t m) {
    return random_impl(0, m - 1);
}

size_t random(size_t m, size_t n) {
    return random_impl(m, n);
}

}  // namespace lc
