#include "lcrypt/hex.h"
#include <array>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <hwy/contrib/algo/transform-inl.h>
#include <hwy/highway.h>
#include <string.h>

namespace unsimd {
std::string hex_marshal(std::string_view what) {
    static char hex[]   = "0123456789abcdef";
    size_t sz           = what.size();
    const uint8_t* text = (const uint8_t*)(what.data());
    std::string out(sz * 2, '\0');
    char* buffer = &out[0];
    int i;
    for (i = 0; i < (int)sz; i++) {
        buffer[i * 2]     = hex[text[i] >> 4];
        buffer[i * 2 + 1] = hex[text[i] & 0xf];
    }
    return out;
}

#define HEX(v, c)                              \
    {                                          \
        char tmp = (char)c;                    \
        if (tmp >= '0' && tmp <= '9') {        \
            v = tmp - '0';                     \
        } else if (tmp >= 'A' && tmp <= 'F') { \
            v = tmp - 'A' + 10;                \
        } else {                               \
            v = tmp - 'a' + 10;                \
        }                                      \
    }

std::string hex_unmarshal(std::string_view what) {
    size_t sz        = what.size();
    const char* text = what.data();
    if (sz & 1) {
        throw std::runtime_error("Invalid hex text size");
    }
    std::string out(sz / 2, '\0');
    char* buffer = &out[0];
    int i;
    for (i = 0; i < (int)sz; i += 2) {
        uint8_t hi, low;
        HEX(hi, text[i]);
        HEX(low, text[i + 1]);
        if (hi > 16 || low > 16) {
            fprintf(stderr, "hi:%d, lo:%d, %c\n", hi, low, text[i + 1]);
            throw std::runtime_error("Invalid hex text");
        }
        buffer[i / 2] = hi << 4 | low;
    }
    return out;
}
}  // namespace unsimd

namespace hn = hwy::HWY_NAMESPACE;

using vec_t = hn::Vec<HWY_FULL(uint8_t)>;

class hex {
    inline static HWY_FULL(uint8_t) _d8{};
    inline static size_t N        = hn::Lanes(_d8);
    inline static const auto _ch0 = hn::Set(_d8, '0');
    inline static const auto _ch9 = hn::Set(_d8, '9');
    inline static const auto _cha = hn::Set(_d8, 'a');
    inline static const auto _chf = hn::Set(_d8, 'f');
    inline static const auto _chA = hn::Set(_d8, 'A');
    inline static const auto _chF = hn::Set(_d8, 'F');
    inline static const auto _9   = hn::Set(_d8, 9);
    inline static const auto _f   = hn::Set(_d8, 0xF);

    alignas(16) inline static const uint8_t hex_table[16] = {  //
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    inline static const auto hex_lookup                   = hn::LoadDup128(_d8, &hex_table[0]);

    // clang-format off
    inline static const auto _lookup_shift_lut = hn::Dup128VecFromValues(_d8,
        /* 0 */ 0x10,        /* 1 */ 0x00,        /* 2 */ 0x00,        /* 3 */ 0x00 - 0x30,
        /* 4 */ 0x0A - 0x41, /* 5 */ 0x00,        /* 6 */ 0x0A - 0x61, /* 7 */ 0x00,
        /* 8 */ 0x00,        /* 9 */ 0x00,        /* a */ 0x00,        /* b */ 0x00,
        /* c */ 0x00,        /* d */ 0x00,        /* e */ 0x00,        /* f */ 0x00
    );
    // clang-format on

    // template <typename T>
    // static inline bool is_hex_helper(const T& v) {
    //     auto in_09 = hn::And(hn::Ge(v, _ch0), hn::Le(v, _ch9));
    //     auto in_AF = hn::And(hn::Ge(v, _chA), hn::Le(v, _chF));
    //     auto in_af = hn::And(hn::Ge(v, _cha), hn::Le(v, _chf));
    //     return hn::AllTrue(_d8, hn::Or(hn::Or(in_09, in_AF), in_af));
    // }

    static inline vec_t lookup_pshufb(const vec_t& xx, int n) {
        const auto higher_nibble = hn::ShiftRightSame(xx, 4);
        const auto result = hn::Add(xx, hn::TableLookupBytes(_lookup_shift_lut, higher_nibble));
        auto idx          = hn::FindFirstTrue(_d8, hn::Gt(result, _f));
        if (HWY_UNLIKELY(idx != -1 && idx < n)) {
            throw lc::input_error(idx, 0);
        }
        return result;
    }

    // xn ==> ASCII 0-9A-Fa-f
    // After transfer: 0-15
    static inline void decode_convert(int offset, int n, vec_t& x1, vec_t& x2) {
        try {
            x1 = lookup_pshufb(x1, n);
        } catch (const lc::input_error& e) {
            const auto shift = e.offset();
            throw lc::input_error(offset + shift * 2, hn::ExtractLane(x1, shift));
        }
        try {
            x2 = lookup_pshufb(x2, n);
        } catch (const lc::input_error& e) {
            const auto shift = e.offset();
            throw lc::input_error(offset + shift * 2 + 1, hn::ExtractLane(x2, shift));
        }
    }

public:
    // static bool is_hex(std::string_view in) {
    //     auto len = in.size();
    //     if (HWY_UNLIKELY(len & 1)) {
    //         return false;
    //     }
    //
    //     const uint8_t* src = reinterpret_cast<const uint8_t*>(in.data());
    //     size_t idx         = 0;
    //     if (len >= N) {
    //         for (; idx <= len - N; idx += N) {
    //             const auto v = hn::LoadU(_d8, src + idx);
    //             if (HWY_UNLIKELY(!is_hex_helper(v))) return false;
    //         }
    //     }
    //
    //     // `count` was a multiple of the vector length `N`: already done.
    //     if (HWY_UNLIKELY(idx == len)) return true;
    //
    //     const size_t remaining = len - idx;
    //     HWY_DASSERT(0 != remaining && remaining < N);
    //     const auto v = hn::LoadNOr(_ch9, _d8, src + idx, remaining);
    //     return is_hex_helper(v);
    // }

    static std::string encode(std::string_view in) {
        static constexpr auto func = [&](const auto& v, auto& hi, auto& lo) {
            auto hi_nibble = hn::ShiftRight<4>(v);  // High 4 bits
            auto lo_nibble = hn::And(v, _f);        // Low 4 bits
            hi             = hn::TableLookupBytes(hex_lookup, hi_nibble);
            lo             = hn::TableLookupBytes(hex_lookup, lo_nibble);
        };

        size_t len = in.size();
        std::string result(len * 2, '\0');
        uint8_t* dest      = reinterpret_cast<uint8_t*>(&result[0]);
        const uint8_t* src = reinterpret_cast<const uint8_t*>(in.data());
        const size_t step  = N;
        size_t idx         = 0;

        auto xx = hn::Zero(_d8);
        auto hi = hn::Zero(_d8);
        auto lo = hn::Zero(_d8);

        if (len >= 4 * step) {
            auto xx1 = hn::Zero(_d8);
            auto hi1 = hn::Zero(_d8);
            auto lo1 = hn::Zero(_d8);
            auto xx2 = hn::Zero(_d8);
            auto hi2 = hn::Zero(_d8);
            auto lo2 = hn::Zero(_d8);
            auto xx3 = hn::Zero(_d8);
            auto hi3 = hn::Zero(_d8);
            auto lo3 = hn::Zero(_d8);

            while (idx + 4 * step - 1 < len) {
                xx = hn::LoadN(_d8, src + idx, step);
                idx += step;
                xx1 = hn::LoadN(_d8, src + idx, step);
                idx += step;
                xx2 = hn::LoadN(_d8, src + idx, step);
                idx += step;
                xx3 = hn::LoadN(_d8, src + idx, step);
                idx -= 3 * step;

                func(xx, hi, lo);
                func(xx1, hi1, lo1);
                func(xx2, hi2, lo2);
                func(xx3, hi3, lo3);

                hn::StoreInterleaved2(hi, lo, _d8, dest + idx * 2);
                idx += step;
                hn::StoreInterleaved2(hi1, lo1, _d8, dest + idx * 2);
                idx += step;
                hn::StoreInterleaved2(hi2, lo2, _d8, dest + idx * 2);
                idx += step;
                hn::StoreInterleaved2(hi3, lo3, _d8, dest + idx * 2);
                idx += step;
            }
        }

        while (idx + step - 1 < len) {
            xx = hn::LoadN(_d8, src + idx, step);
            func(xx, hi, lo);
            hn::StoreInterleaved2(hi, lo, _d8, dest + idx * 2);
            idx += step;
        }

        if (HWY_LIKELY(idx != len)) {
            const size_t remaining = len - idx;
            xx                     = hn::LoadN(_d8, src + idx, remaining);
            func(xx, hi, lo);
            uint8_t buf[N * 2] = {0};
            hn::StoreInterleaved2(hi, lo, _d8, buf);
            memcpy(dest + idx * 2, buf, remaining * 2);
        }

        return result;
    }

    static std::string decode(std::string_view in) {
        size_t len = in.size();
        if (HWY_UNLIKELY(len & 1)) {
            throw std::runtime_error("Invalid hex text size");
        }

        std::string out(len / 2, '\0');
        const uint8_t* src = reinterpret_cast<const uint8_t*>(in.data());
        uint8_t* dest      = reinterpret_cast<uint8_t*>(out.data());
        const size_t step  = N * 2;
        size_t idx         = 0;

        auto xx1 = hn::Zero(_d8);
        auto xx2 = hn::Zero(_d8);
        auto yy  = hn::Zero(_d8);

        if (len >= 2 * step) {
            auto xx3 = hn::Zero(_d8);
            auto xx4 = hn::Zero(_d8);
            auto yy2 = hn::Zero(_d8);

            while (idx + 2 * step - 1 < len) {
                hn::LoadInterleaved2(_d8, src + idx, xx1, xx2);
                idx += step;
                hn::LoadInterleaved2(_d8, src + idx, xx3, xx4);
                idx -= step;

                decode_convert(idx, N, xx1, xx2);
                yy = hn::Or(hn::ShiftLeftSame(xx1, 4), xx2);
                decode_convert(idx + step, N, xx3, xx4);
                yy2 = hn::Or(hn::ShiftLeftSame(xx3, 4), xx4);

                hn::StoreU(yy, _d8, dest + idx / 2);
                idx += step;
                hn::StoreU(yy2, _d8, dest + idx / 2);
                idx += step;
            }
        }

        while (idx + step - 1 < len) {
            hn::LoadInterleaved2(_d8, src + idx, xx1, xx2);
            decode_convert(idx, N, xx1, xx2);
            yy = hn::Or(hn::ShiftLeftSame(xx1, 4), xx2);
            hn::StoreU(yy, _d8, dest + idx / 2);
            idx += step;
        }

        if (HWY_LIKELY(idx != len)) {
            const size_t remaining = len - idx;
            hn::LoadInterleaved2(_d8, src + idx, xx1, xx2);
            decode_convert(idx, remaining / 2, xx1, xx2);
            yy = hn::Or(hn::ShiftLeftSame(xx1, 4), xx2);
            hn::StoreN(yy, _d8, dest + idx / 2, remaining / 2);
        }

        return out;
    }
};

namespace lc {

std::string hex_encode(const char* buf, size_t len) {
    auto s = std::string_view(buf, len);
    if (len <= 8) {
        return unsimd::hex_marshal(s);
    } else {
        return hex::encode(s);
    }
}

std::string hex_decode(const char* buf, size_t len) {
    auto s = std::string_view(buf, len);
    if (len <= 8) {
        return unsimd::hex_unmarshal(s);
    } else {
        return hex::decode(s);
    }
}

}  // namespace lc
