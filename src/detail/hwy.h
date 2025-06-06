#pragma once

#include <hwy/highway.h>
#include <stdint.h>

namespace hn = hwy::HWY_NAMESPACE;

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;

using vu8  = hn::Vec<HWY_FULL(u8)>;
using vu16 = hn::Vec<HWY_FULL(u16)>;

static constexpr HWY_FULL(u8) _du8{};
static constexpr HWY_FULL(u16) _du16{};
static constexpr HWY_FULL(u32) _du32{};
static constexpr HWY_FULL(i8) _di8{};
static constexpr HWY_FULL(i16) _di16{};
static constexpr HWY_FULL(i32) _di32{};

static constexpr size_t N8  = hn::Lanes(_du8);
static constexpr size_t N16 = hn::Lanes(_du16);
static constexpr size_t N32 = hn::Lanes(_du32);

#define HWY_CLAMP(x, min, max) (HWY_MAX(HWY_MIN((x), (max)), (min)))

template <typename D>
struct hwy_tag_inner;

// ScalableTag
template <typename T, size_t N, int Pow>
struct hwy_tag_inner<hn::Simd<T, N, Pow>> {
    using type = T;
};

template <typename D>
hn::Vec<D> Lt(D d, const hn::Vec<D> x, const hn::Vec<D> y) {
    using T            = typename hwy_tag_inner<D>::type;
    constexpr auto max = hwy::LimitsMax<T>();
    const auto diff    = hn::Sub(hn::Set(d, max), y);
    const auto v       = hn::SaturatedSub(hn::SaturatedAdd(x, diff), hn::Set(d, max - 1));
    return hn::Sub(v, hn::Set(d, 1));
}

template <typename D>
hn::Vec<D> Gt(D d, const hn::Vec<D> x, const hn::Vec<D> y) {
    return Lt(d, y, x);
}

template <typename D>
hn::Vec<D> Ne(D d, const hn::Vec<D> x, const hn::Vec<D> y) {
    return hn::Or(Lt(d, x, y), Gt(d, x, y));
}

template <typename D>
hn::Vec<D> Eq(D d, const hn::Vec<D> x, const hn::Vec<D> y) {
    return hn::Not(Ne(d, x, y));
}

template <typename V>
V IfThenElse(const V mask, const V yes, const V no) {
    const auto y = hn::And(mask, yes);
    const auto n = hn::AndNot(mask, no);
    return hn::Or(y, n);
}

template <typename V>
V IfThenElseZero(const V mask, const V yes) {
    return hn::And(mask, yes);
}

template <typename V>
V IfThenZeroElse(const V mask, const V no) {
    return hn::AndNot(mask, no);
}
