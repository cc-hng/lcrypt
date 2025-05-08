#include "lcrypt/base64.h"
#include <array>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <hwy/aligned_allocator.h>
#include <hwy/contrib/algo/copy-inl.h>
#include <hwy/contrib/algo/transform-inl.h>
#include <hwy/highway.h>
#include <string.h>

#define HWY_CLAMP(x, min, max) (HWY_MAX(HWY_MIN((x), (max)), (min)))

namespace hn = hwy::HWY_NAMESPACE;

class base64 {
    inline static HWY_FULL(uint8_t) _d8;
    inline static HWY_FULL(uint16_t) _d16;
    inline static HWY_FULL(uint32_t) _d32;
    inline static HWY_FULL(int8_t) _di8;
    inline static HWY_FULL(int16_t) _di16;
    inline static HWY_FULL(int32_t) _di32;
    inline static constexpr size_t N8 = hn::Lanes(_d8);

    // const var
    inline static const auto _0x0fc0fc00 = hn::BitCast(_d8, hn::Set(_d32, 0x0fc0fc00));
    inline static const auto _0x04000040 = hn::BitCast(_d16, hn::Set(_d32, 0x04000040));
    inline static const auto _0x003f03f0 = hn::BitCast(_d8, hn::Set(_d32, 0x003f03f0));
    inline static const auto _0x01000010 = hn::BitCast(_d16, hn::Set(_d32, 0x01000010));
    inline static const auto _0x01400140 = hn::BitCast(_d8, hn::Set(_d32, 0x01400140));
    inline static const auto _0x00011000 = hn::BitCast(_d8, hn::Set(_d32, 0x00011000));
    inline static const auto _0          = hn::Set(_d8, 0);
    inline static const auto _13         = hn::Set(_d8, 13);
    inline static const auto _26         = hn::Set(_d8, 26);
    inline static const auto _51         = hn::Set(_d8, 51);
    inline static const auto _0x2f       = hn::Set(_d8, 0x2f);

private:
    template <typename T>
    static auto encode_helper(const T& v) {
        // 128 | 256 | 512 simd vector
        alignas(16) static constexpr uint8_t shuf_buf[] = {
            1,  0,  2,  1,  4,  3,  5,  4,  7,  6,  8,  7,  10, 9,  11, 10,  //
            17, 16, 18, 17, 20, 19, 21, 20, 23, 22, 24, 23, 26, 25, 27, 26,  //
            33, 32, 34, 33, 36, 35, 37, 36, 39, 38, 40, 39, 42, 41, 43, 42,  //
            49, 48, 50, 49, 52, 51, 53, 52, 55, 54, 56, 55, 58, 57, 59, 58,  //
        };
        static const auto shuf = hn::LoadU(_d8, shuf_buf);

        // refer:
        // https://github.com/WojciechMula/base64simd/blob/master/encode/encode.sse.cpp
        const auto in      = hn::TableLookupBytes(v, shuf);
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

        static const auto lut = hn::Dup128VecFromValues(_d8, 'A', '0' - 52, '0' - 52, '0' - 52,
                                                        '0' - 52, '0' - 52, '0' - 52, '0' - 52,
                                                        '0' - 52, '0' - 52, '0' - 52, '+' - 62,
                                                        '/' - 63, 'a' - 26, 0, 0);
        result                = hn::TableLookupBytes(lut, result);
        return hn::Add(result, indices);
    }

    static auto loadone(const uint8_t* src, int count) {
        // every 128bit
        //      12 : 16
        constexpr size_t multiple = N8 / 16;
        if constexpr (multiple == 4) {
            auto p1 = hn::LoadN(_d8, src, HWY_MIN(12, count));
            count -= 12;
            auto p2 = hn::LoadN(_d8, src + 12, HWY_CLAMP(count, 0, 12));
            count -= 12;
            auto p3 = hn::LoadN(_d8, src + 24, HWY_CLAMP(count, 0, 12));
            count -= 12;
            auto p4 = hn::LoadN(_d8, src + 36, HWY_CLAMP(count, 0, 12));
            return hn::Add(hn::Add(hn::Add(p1, hn::SlideUpLanes(_d8, p2, 16)),
                                   hn::SlideUpLanes(_d8, p3, 32)),
                           hn::SlideUpLanes(_d8, p4, 48));
        } else if constexpr (multiple == 2) {
            auto p1 = hn::LoadN(_d8, src, HWY_MIN(12, count));
            count -= 12;
            auto p2 = hn::LoadN(_d8, src + 12, HWY_CLAMP(count, 0, 12));
            return hn::Add(p1, hn::SlideUpLanes(_d8, p2, 16));
        } else if constexpr (multiple == 1) {
            return hn::LoadN(_d8, src, count);
        } else {
            throw std::runtime_error("Unknown lanes!!!");
        }
    }

    static inline size_t padding_count(std::string_view in) {
        size_t padding = 0;
        for (auto it = in.rbegin(); it != in.rend() && *it == '='; ++it, ++padding)
            ;
        return padding;
    }

    template <typename T>
    static auto decode_lookup(const T& xx, int n) {
        // refer:
        // https://github.com/WojciechMula/base64simd/blob/master/decode/lookup.sse.cpp
        static constexpr uint8_t linv = 1;
        static constexpr uint8_t hinv = 0;
        // clang-format off
        static const auto lower_bound_lut = hn::Dup128VecFromValues(_d8,
            /* 0 */ linv, /* 1 */ linv, /* 2 */ 0x2b, /* 3 */ 0x30,
            /* 4 */ 0x41, /* 5 */ 0x50, /* 6 */ 0x61, /* 7 */ 0x70,
            /* 8 */ linv, /* 9 */ linv, /* a */ linv, /* b */ linv,
            /* c */ linv, /* d */ linv, /* e */ linv, /* f */ linv
        );
        static const auto upper_bound_lut = hn::Dup128VecFromValues(_d8,
            /* 0 */ hinv, /* 1 */ hinv, /* 2 */ 0x2b, /* 3 */ 0x39,
            /* 4 */ 0x4f, /* 5 */ 0x5a, /* 6 */ 0x6f, /* 7 */ 0x7a,
            /* 8 */ hinv, /* 9 */ hinv, /* a */ hinv, /* b */ hinv,
            /* c */ hinv, /* d */ hinv, /* e */ hinv, /* f */ hinv
        );
        static const auto shift_lut = hn::Dup128VecFromValues(_d8,
            /* 0 */ 0x00,        /* 1 */ 0x00,        /* 2 */ 0x3e - 0x2b, /* 3 */ 0x34 - 0x30,
            /* 4 */ 0x00 - 0x41, /* 5 */ 0x0f - 0x50, /* 6 */ 0x1a - 0x61, /* 7 */ 0x29 - 0x70,
            /* 8 */ 0x00,        /* 9 */ 0x00,        /* a */ 0x00,        /* b */ 0x00,
            /* c */ 0x00,        /* d */ 0x00,        /* e */ 0x00,        /* f */ 0x00
        );
        // clang-format on

        const auto higher_nibble = hn::ShiftRightSame(xx, 4);

        const auto lower_bound = hn::TableLookupBytes(lower_bound_lut, higher_nibble);
        const auto upper_bound = hn::TableLookupBytes(upper_bound_lut, higher_nibble);
        const auto below       = hn::Lt(xx, lower_bound);
        const auto above       = hn::Gt(xx, upper_bound);
        const auto eq_2f       = hn::Eq(xx, _0x2f);

        const auto outside = hn::AndNot(eq_2f, hn::Or(below, above));
        const auto idx     = hn::FindFirstTrue(_d8, hn::And(outside, hn::FirstN(_d8, n)));
        if (HWY_UNLIKELY(idx != -1)) {
            throw lc::input_error(idx, 0);
        }

        const auto shift = hn::TableLookupBytes(shift_lut, higher_nibble);
        const auto t0    = hn::Add(xx, shift);
        const auto result =
            hn::Add(t0, hn::And(hn::VecFromMask(_d8, eq_2f), hn::Set(_d8, (uint8_t)-3)));
        return result;
    }

    template <typename T>
    static inline auto decode_helper(const T& v) {
        alignas(16) static constexpr uint8_t lut_buf[] = {
            2,  1,  0,  6,  5,  4,  10, 9,  8,  14, 13, 12, 3,  7,  11, 15,  //
            18, 17, 16, 22, 21, 20, 26, 25, 24, 30, 29, 28, 19, 23, 27, 31,  //
            34, 33, 32, 38, 37, 36, 42, 41, 40, 46, 45, 44, 35, 39, 43, 47,  //
            50, 49, 48, 54, 53, 52, 58, 57, 56, 62, 61, 60, 51, 55, 59, 63,  //
        };
        static const auto lut = hn::LoadU(_d8, lut_buf);

        const auto merged = hn::SatWidenMulPairwiseAdd(_di16, v, hn::BitCast(_di8, _0x01400140));
        const auto packed = hn::WidenMulPairwiseAdd(_d32, hn::BitCast(_d16, merged),
                                                    hn::BitCast(_d16, _0x00011000));
        auto result       = hn::TableLookupBytes(hn::BitCast(_d8, packed), lut);
        return result;
    }

public:
    static std::string encode(std::string_view in) {
        size_t len = in.size();
        std::string result(((len + 2) / 3) * 4, '\0');
        uint8_t* dest      = reinterpret_cast<uint8_t*>(&result[0]);
        const uint8_t* src = reinterpret_cast<const uint8_t*>(in.data());

        const size_t step = (N8 / 4) * 3;
        size_t idx = 0, j = 0;
        auto xx = hn::Zero(_d8);
        auto yy = hn::Zero(_d8);

        while (idx + step - 1 < len) {
            xx = loadone(src + idx, step);
            yy = encode_helper(xx);
            hn::StoreU(yy, _d8, dest + j);
            idx += step;
            j += N8;
        }

        if (HWY_LIKELY(idx != len)) {
            const size_t remaining = len - idx;
            xx                     = loadone(src + idx, remaining);
            yy                     = encode_helper(xx);
            hn::StoreN(yy, _d8, dest + j, ((remaining + 2) / 3 * 4));
            // ==
            size_t mod = remaining % 3;
            if (mod == 1) {
                auto it = result.rbegin();
                *it     = '=';
                *(++it) = '=';
            } else if (mod == 2) {
                result.back() = '=';
            }
        }

        return result;
    }

    static std::string decode(std::string_view in) {
        size_t len         = in.size();
        size_t num_padding = padding_count(in);
        std::string result((len / 4) * 3 - num_padding, '\0');
        len -= num_padding;
        uint8_t* dest      = reinterpret_cast<uint8_t*>(&result[0]);
        const uint8_t* src = reinterpret_cast<const uint8_t*>(in.data());
        size_t idx = 0, j = 0;
        const size_t step  = N8;
        const size_t step0 = (step / 4) * 3;

        auto xx = hn::Zero(_d8);
        auto yy = hn::Zero(_d8);
        while (idx + step - 1 < len) {
            xx = hn::LoadN(_d8, src + idx, step);
            try {
                yy = decode_lookup(xx, step);
            } catch (const lc::input_error& e) {
                const auto shift = e.offset();
                throw lc::input_error(idx + shift, src[idx + shift]);
            }
            yy = decode_helper(yy);

            for (int i = 0; i < N8 / 16; i++, j += 12) {
                if (i) {
                    yy = hn::SlideDownLanes(_d8, yy, i * 16);
                }
                hn::StoreN(yy, _d8, dest + j, 12);
            }
            idx += step;
        }

        if (HWY_LIKELY(idx != len)) {
            const size_t remaining = len - idx;
            size_t left            = result.size() - j;
            xx                     = hn::LoadN(_d8, src + idx, remaining);
            try {
                yy = decode_lookup(xx, remaining);
            } catch (const lc::input_error& e) {
                const auto shift = e.offset();
                throw lc::input_error(idx + shift, src[idx + shift]);
            }
            yy = decode_helper(yy);

            for (int i = 0; i < N8 / 16 && left; i++, j += 12, left -= 12) {
                if (i) {
                    yy = hn::SlideDownLanes(_d8, yy, i * 16);
                }
                hn::StoreN(yy, _d8, dest + j, HWY_MIN(left, 12));
            }
        }

        return result;
    }
};

namespace lc {

std::string base64_encode(const char* buf, size_t len) {
    return base64::encode(std::string_view(buf, len));
}

std::string base64_decode(const char* buf, size_t len) {
    return base64::decode(std::string_view(buf, len));
}

}  // namespace lc
