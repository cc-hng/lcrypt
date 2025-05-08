#pragma once

#include <functional>
#include <list>
#include <nanobench.h>

#define CC_CONCAT0(a, b) a##b
#define CC_CONCAT(a, b)  CC_CONCAT0(a, b)
#define CC_CALL_OUTSIDE(fn) \
    [[maybe_unused]] static const bool CC_CONCAT(__b_, __LINE__) = ((fn), true)

namespace bench = ankerl::nanobench;

class BenchRegistry {
    using BenchFn = std::function<void(bench::Bench&)>;

public:
    static BenchRegistry& get() {
        static BenchRegistry br;
        return br;
    }

    template <typename Fn>
    void registe(Fn&& fn) {
        bench_list_.emplace_back(std::forward<Fn>(fn));
    }

    void run() {
        for (const auto& f : bench_list_) {
            f(b_);
        }
    }

private:
    BenchRegistry() = default;

private:
    bench::Bench b_;
    std::list<BenchFn> bench_list_;
};

#define BENCHMARK_REGISTE(fn) CC_CALL_OUTSIDE(BenchRegistry::get().registe(fn))
