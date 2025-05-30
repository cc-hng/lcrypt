#include "lcrypt/base64.h"
#include <stdexcept>
#include <string>
#include <hwy/contrib/unroller/unroller-inl.h>
#include <hwy/highway.h>
#include <string.h>

#define HWY_CLAMP(x, min, max) (HWY_MAX(HWY_MIN((x), (max)), (min)))

namespace hn = hwy::HWY_NAMESPACE;
using vec_t  = hn::Vec<HWY_FULL(uint8_t)>;
using u8     = uint8_t;
using u16    = uint16_t;
using u32    = uint32_t;
using i8     = int8_t;
using i16    = int16_t;

namespace {

static inline size_t base64_padding_count(const char* buf, size_t len) {
    size_t padding = 0;
    for (int i = len - 1; i >= 0 && buf[i] == '='; --i, ++padding)
        ;
    return padding;
}

struct EncodeUnit : hn::UnrollerUnit<EncodeUnit, u8, u8> {
    using D = hn::ScalableTag<u8>;
    static constexpr D _d8{};
    static constexpr size_t N8 = hn::Lanes(_d8);
    static constexpr HWY_FULL(u16) _d16{};
    static constexpr HWY_FULL(u32) _d32{};
    const vec_t _0x0fc0fc00                  = hn::BitCast(_d8, hn::Set(_d32, 0x0fc0fc00));
    const hn::Vec<HWY_FULL(u16)> _0x04000040 = hn::BitCast(_d16, hn::Set(_d32, 0x04000040));
    const vec_t _0x003f03f0                  = hn::BitCast(_d8, hn::Set(_d32, 0x003f03f0));
    const hn::Vec<HWY_FULL(u16)> _0x01000010 = hn::BitCast(_d16, hn::Set(_d32, 0x01000010));
    const vec_t _0                           = hn::Set(_d8, 0);
    const vec_t _13                          = hn::Set(_d8, 13);
    const vec_t _26                          = hn::Set(_d8, 26);
    const vec_t _51                          = hn::Set(_d8, 51);

    // clang-format off
    HWY_ALIGN static constexpr uint8_t _encode_shuf_buf[] = {
        5,  4,  6,  5,  8,  7,  9,  8,  11, 10, 12, 11, 14, 13, 15, 14,  //
        17, 16, 18, 17, 20, 19, 21, 20, 23, 22, 24, 23, 26, 25, 27, 26,  //
        37, 36, 38, 37, 40, 39, 41, 40, 43, 42, 44, 33, 46, 45, 47, 46,  //
        49, 48, 50, 49, 52, 51, 53, 52, 55, 54, 56, 55, 58, 57, 59, 58,  //
    };
    const vec_t _encode_indices = hn::LoadU(_d8, _encode_shuf_buf);
    const vec_t _encode_lut =
        hn::Dup128VecFromValues(_d8, 'A', '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
                                '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '+' - 62,
                                '/' - 63, 'a' - 26, 0, 0);
    // clang-format on

    hn::Vec<D> Func(ptrdiff_t, const hn::Vec<D> xx, const hn::Vec<D>) {
        // refer:
        // https://github.com/WojciechMula/base64simd/blob/master/encode/encode.sse.cpp
        const auto in      = hn::TableLookupBytes(xx, _encode_indices);
        const auto t0      = hn::And(in, _0x0fc0fc00);
        const auto t1      = hn::MulHigh(hn::BitCast(_d16, t0), _0x04000040);
        const auto t2      = hn::And(in, _0x003f03f0);
        const auto t3      = hn::Mul(hn::BitCast(_d16, t2), _0x01000010);
        const auto indices = hn::Or(hn::BitCast(_d8, t1), hn::BitCast(_d8, t3));

        // refer:
        // https://github.com/WojciechMula/base64simd/blob/master/encode/lookup.sse.cpp
        auto result     = hn::SaturatedSub(indices, _51);
        const auto less = hn::IfThenZeroElse(hn::Gt(_26, indices), _13);
        result          = hn::IfThenElse(hn::Gt(result, _0), result, less);

        result = hn::TableLookupBytes(_encode_lut, result);
        return hn::Add(result, indices);
    }

    hn::Vec<D> LoadImpl(const ptrdiff_t idx, const u8* from) {
        constexpr size_t multiple = N8 / 16;
        constexpr size_t count    = N8 * 3 / 4;

        /// indexof(src):indexof(dest) => 3:4
        ptrdiff_t j = idx * 3 / 4;
        if constexpr (multiple == 4) {
            auto p1 = hn::LoadN(_d8, from + j - 4, count + 4);
            auto p2 = hn::LoadN(_d8, from + j + count - 4, count + 4);
            return hn::Add(p1, hn::SlideUpLanes(_d8, p2, 32));
        } else if constexpr (multiple == 2 || multiple == 1) {
            return hn::LoadN(_d8, from + j - 4, count + 4);
        } else {
            throw std::runtime_error("Unsupported lanes!!! sizeof(lanes) = " + std::to_string(N8));
        }
    }

    hn::Vec<D> MaskLoadImpl(const ptrdiff_t idx, const u8* from, const ptrdiff_t places) {
        /// convert neg places
        if (places < 0) {
            return me()->LoadImpl(idx + places + N8, from);
        } else {
            return me()->LoadImpl(idx, from);
        }
    }

    ptrdiff_t
    MaskStoreImpl(const ptrdiff_t idx, u8* to, const hn::Vec<D> x, const ptrdiff_t places) {
        ptrdiff_t i = idx;
        ptrdiff_t p = std::abs(places);
        if (places < 0) {
            i = idx + places + N8;
        }
        hn::BlendedStore(x, hn::FirstN(_d8, p), _d8, to + i);
        return p;
    }
};

struct DecodeUnit : hn::UnrollerUnit<DecodeUnit, u8, u8> {
    using D = hn::ScalableTag<u8>;
    static constexpr D _d8{};
    static constexpr HWY_FULL(u16) _d16{};
    static constexpr HWY_FULL(u32) _d32{};
    static constexpr HWY_FULL(i8) _di8{};
    static constexpr HWY_FULL(i16) _di16{};
    static constexpr size_t N8 = hn::Lanes(_d8);

    const hn::Vec<D> _3          = hn::Set(_d8, 3);
    const hn::Vec<D> _0x2f       = hn::Set(_d8, 0x2f);
    const hn::Vec<D> _0x40       = hn::Set(_d8, 0x40);
    const hn::Vec<D> _0x01400140 = hn::BitCast(_d8, hn::Set(_d32, 0x01400140));
    const hn::Vec<D> _0x00011000 = hn::BitCast(_d8, hn::Set(_d32, 0x00011000));
    // clang-format off
    HWY_ALIGN static constexpr uint8_t _lut_buf[] = {
        2,  1,  0,  6,  5,  4,  10, 9,  8,  14, 13, 12, 3,  7,  11, 15,  //
        18, 17, 16, 22, 21, 20, 26, 25, 24, 30, 29, 28, 19, 23, 27, 31,  //
        34, 33, 32, 38, 37, 36, 42, 41, 40, 46, 45, 44, 35, 39, 43, 47,  //
        50, 49, 48, 54, 53, 52, 58, 57, 56, 62, 61, 60, 51, 55, 59, 63,  //
    };
    const hn::Vec<D> _lut = hn::LoadU(_d8, _lut_buf);
    const hn::Vec<D> _shift_lut = hn::Dup128VecFromValues(_d8,
        /* 0 */ 0x60,        /* 1 */ 0x60,        /* 2 */ 0x3e - 0x2b, /* 3 */ 0x34 - 0x30,
        /* 4 */ 0x00 - 0x41, /* 5 */ 0x0f - 0x50, /* 6 */ 0x1a - 0x61, /* 7 */ 0x29 - 0x70,
        /* 8 */ 0x60,        /* 9 */ 0x60,        /* a */ 0x60,        /* b */ 0x60,
        /* c */ 0x60,        /* d */ 0x60,        /* e */ 0x60,        /* f */ 0x60
    );

    static constexpr u8 linv = 1;
    static constexpr u8 hinv = 0;
    const hn::Vec<D> _lower_lut = hn::Dup128VecFromValues(_d8,
        /* 0 */ linv, /* 1 */ linv, /* 2 */ 0x2b, /* 3 */ 0x30,
        /* 4 */ 0x41, /* 5 */ 0x50, /* 6 */ 0x61, /* 7 */ 0x70,
        /* 8 */ linv, /* 9 */ linv, /* a */ linv, /* b */ linv,
        /* c */ linv, /* d */ linv, /* e */ linv, /* f */ linv
    );
    const hn::Vec<D> _upper_lut = hn::Dup128VecFromValues(_d8,
        /* 0 */ hinv, /* 1 */ hinv, /* 2 */ 0x2b, /* 3 */ 0x39,
        /* 4 */ 0x4f, /* 5 */ 0x5a, /* 6 */ 0x6f, /* 7 */ 0x7a,
        /* 8 */ hinv, /* 9 */ hinv, /* a */ hinv, /* b */ hinv,
        /* c */ hinv, /* d */ hinv, /* e */ hinv, /* f */ hinv
    );

    // clang-format on

    std::string_view _in;   // original input
    const size_t _padding;  // padding count of the input
    int _idx    = 0;
    int _places = 0;  // places to store, positive means top, negative means bottom
    DecodeUnit(std::string_view in, size_t padding) : _in(in), _padding(padding) {}

    inline ptrdiff_t adjust_index(ptrdiff_t idx) const {
        const size_t len = _in.size();
        if (idx + N8 == len) {
            return len - (len % 32);
        } else {
            return idx;
        }
    }

    hn::Vec<D> Func(ptrdiff_t, const hn::Vec<D> xx, const hn::Vec<D> yy) {
        const size_t len = _in.size();
        /// lookup
        // refer:
        // https://github.com/WojciechMula/base64simd/blob/master/decode/lookup.sse.cpp
        const auto higher_nibble = hn::ShiftRightSame(xx, 4);
        const auto shift         = hn::TableLookupBytes(_shift_lut, higher_nibble);
        const auto eq_2f         = hn::Eq(xx, _0x2f);
        const auto t0            = hn::Add(xx, shift);
        auto result              = hn::MaskedSubOr(t0, eq_2f, t0, _3);

        /// check validity
        const auto below   = hn::Lt(xx, hn::TableLookupBytes(_lower_lut, higher_nibble));
        const auto above   = hn::Gt(xx, hn::TableLookupBytes(_upper_lut, higher_nibble));
        const auto outside = hn::AndNot(eq_2f, hn::Or(below, above));
        int j              = hn::FindFirstTrue(_d8, outside);
        if (HWY_UNLIKELY(j != -1 && j < _places)) {
            throw lc::input_error(j + _idx, _in[j + _idx]);
        }

        /// decode
        const auto merged =
            hn::SatWidenMulPairwiseAdd(_di16, result, hn::BitCast(_di8, _0x01400140));
        const auto packed = hn::WidenMulPairwiseAdd(_d32, hn::BitCast(_d16, merged),
                                                    hn::BitCast(_d16, _0x00011000));
        return hn::TableLookupBytes(hn::BitCast(_d8, packed), _lut);
    }

    hn::Vec<D> MaskLoad(const ptrdiff_t idx, u8* from, const ptrdiff_t places) {
        ptrdiff_t i = idx;
        ptrdiff_t p = places;
        if (places < 0) {
            i = idx + places + N8;
            p = -places;
        }
        _idx    = i;
        _places = p;
        return me()->MaskLoadImpl(i, from, p);
    }

    bool StoreAndShortCircuitImpl(const ptrdiff_t idx, uint8_t* to, const hn::Vec<D> x) {
        /// indexof(src):indexof(dest) => 4:3
        //    src:  x x x o | x x x o
        //    dest: x x x | x x x
        ptrdiff_t j                = idx * 3 / 4;
        constexpr size_t count     = 12;  // 16 * 3 / 4
        constexpr size_t multiples = N8 / 16;
        auto y                     = x;
        for (int i = 0; i < multiples; ++i, j += 12) {
            if (i) {
                y = hn::SlideDownLanes(_d8, y, i * 16);
            }

            hn::StoreN(y, _d8, to + j, count);
        }
        return true;
    }

    ptrdiff_t
    MaskStoreImpl(const ptrdiff_t idx, u8* to, const hn::Vec<D> x, const ptrdiff_t places) {
        /// convert neg places
        ptrdiff_t i = idx;
        ptrdiff_t p = places;
        if (places < 0) {
            i = idx + places + N8;
            p = -places;
        }

        ptrdiff_t j                = i * 3 / 4;
        constexpr size_t count     = 12;  // 16 * 3 / 4
        constexpr size_t multiples = N8 / 16;
        int left                   = p / 4 * 3;  // 4:3
        switch (_padding) {
        case 1: left += 2; break;  // 4:3
        case 2: left += 1; break;  // 4:3
        default: break;            // no padding
        }

        const size_t z = left;
        auto y         = x;
        for (int i = 0; i < multiples && left > 0; ++i, j += count, left -= count) {
            if (i) {
                y = hn::SlideDownLanes(_d8, y, i * 16);
            }
            hn::BlendedStore(y, hn::FirstN(_d8, HWY_MIN(left, count)), _d8, to + j);
        }
        return z;
    }
};

}  // namespace

namespace lc {

std::string base64_encode(const char* in, size_t len) {
    EncodeUnit unit;
    const size_t mod = len % 3;
    size_t olen      = base64_encode_size(in, len);
    std::string result(olen, '\0');
    hn::Unroller(unit, (u8*)(const_cast<char*>(in)), (u8*)result.data(), olen);
    if (mod > 0) {
        // padding
        int pad = 3 - mod;
        for (int i = 0, j = olen - 1; i < pad; ++i, --j) {
            result[j] = '=';
        }
    }
    return result;
}

std::string base64_decode(const char* in, size_t len) {
    const size_t padding = base64_padding_count(in, len);
    size_t olen          = base64_decode_size(in, len);
    std::string result(olen, '\0');
    DecodeUnit unit(std::string_view(in, len - padding), padding);
    hn::Unroller(unit, (u8*)(const_cast<char*>(in)), (u8*)result.data(), len - padding);
    return result;
}

}  // namespace lc
