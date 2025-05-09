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
    inline static const auto _3          = hn::Set(_d8, 3);
    inline static const auto _13         = hn::Set(_d8, 13);
    inline static const auto _26         = hn::Set(_d8, 26);
    inline static const auto _51         = hn::Set(_d8, 51);
    inline static const auto _0x2f       = hn::Set(_d8, 0x2f);
    inline static const auto _0x40       = hn::Set(_d8, 0x40);

    // clang-format off
    inline static const auto _decode_shift_lut = hn::Dup128VecFromValues(_d8,
        /* 0 */ 0x60,        /* 1 */ 0x60,        /* 2 */ 0x3e - 0x2b, /* 3 */ 0x34 - 0x30,
        /* 4 */ 0x00 - 0x41, /* 5 */ 0x0f - 0x50, /* 6 */ 0x1a - 0x61, /* 7 */ 0x29 - 0x70,
        /* 8 */ 0x60,        /* 9 */ 0x60,        /* a */ 0x60,        /* b */ 0x60,
        /* c */ 0x60,        /* d */ 0x60,        /* e */ 0x60,        /* f */ 0x60
    );
    // clang-format on

    // 128 | 256 | 512 simd vector
    inline static constexpr uint8_t _encode_shuf_buf[] = {
        5,  4,  6,  5,  8,  7,  9,  8,  11, 10, 12, 11, 14, 13, 15, 14,  //
        17, 16, 18, 17, 20, 19, 21, 20, 23, 22, 24, 23, 26, 25, 27, 26,  //
        37, 36, 38, 37, 40, 39, 41, 40, 43, 42, 44, 33, 46, 45, 47, 46,  //
        49, 48, 50, 49, 52, 51, 53, 52, 55, 54, 56, 55, 58, 57, 59, 58,  //
    };
    inline static const auto _encode_indices = hn::LoadU(_d8, _encode_shuf_buf);
    inline static const auto _encode_lut =
        hn::Dup128VecFromValues(_d8, 'A', '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52,
                                '0' - 52, '0' - 52, '0' - 52, '0' - 52, '0' - 52, '+' - 62,
                                '/' - 63, 'a' - 26, 0, 0);

private:
    template <typename T>
    static auto encode_helper(const T& v) {
        // refer:
        // https://github.com/WojciechMula/base64simd/blob/master/encode/encode.sse.cpp
        const auto in      = hn::TableLookupBytes(v, _encode_indices);
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

    static inline auto loadone(const uint8_t* src, int count) {
        // every 128bit
        //      12 : 16
        constexpr size_t multiple = N8 / 16;
        if constexpr (multiple == 4) {
            auto p1 = hn::LoadN(_d8, src - 4, HWY_MIN(24, count) + 4);
            count -= 24;
            auto p2 = hn::LoadN(_d8, src + 24 - 4, HWY_CLAMP(count, 0, 24) + 4);
            return hn::Add(p1, hn::SlideUpLanes(_d8, p2, 32));
        } else if constexpr (multiple == 2 || multiple == 1) {
            return hn::LoadN(_d8, src - 4, count + 4);
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
        const auto higher_nibble = hn::ShiftRightSame(xx, 4);
        const auto eq_2f         = hn::Eq(xx, _0x2f);
        const auto shift         = hn::TableLookupBytes(_decode_shift_lut, higher_nibble);
        const auto t0            = hn::Add(xx, shift);

        auto result        = hn::MaskedSubOr(t0, eq_2f, t0, _3);
        const auto outside = hn::Ge(result, _0x40);
        const auto idx     = hn::FindFirstTrue(_d8, outside);
        if (HWY_UNLIKELY(idx != -1 && idx < n)) {
            throw lc::input_error(idx, 0);
        }

        if (idx >= n) {
            result = hn::IfThenZeroElse(outside, result);
        }
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

        if (len >= 4 * step) {
            auto xx1 = hn::Zero(_d8);
            auto yy1 = hn::Zero(_d8);
            auto xx2 = hn::Zero(_d8);
            auto yy2 = hn::Zero(_d8);
            auto xx3 = hn::Zero(_d8);
            auto yy3 = hn::Zero(_d8);

            while (idx + 4 * step - 1 < len) {
                xx = loadone(src + idx, step);
                idx += step;
                xx1 = loadone(src + idx, step);
                idx += step;
                xx2 = loadone(src + idx, step);
                idx += step;
                xx3 = loadone(src + idx, step);
                idx += step;

                yy  = encode_helper(xx);
                yy1 = encode_helper(xx1);
                yy2 = encode_helper(xx2);
                yy3 = encode_helper(xx3);

                hn::StoreU(yy, _d8, dest + j);
                j += N8;
                hn::StoreU(yy1, _d8, dest + j);
                j += N8;
                hn::StoreU(yy2, _d8, dest + j);
                j += N8;
                hn::StoreU(yy3, _d8, dest + j);
                j += N8;
            }
        }

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
