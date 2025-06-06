#include "detail/hwy.h"
#include "lcrypt/hex.h"
#include <stdexcept>
#include <string>
#include <hwy/contrib/unroller/unroller-inl.h>
#include <string.h>

namespace unsimd {

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

void hex__marshal(const char* in, size_t insize, char* out) {
    static char _hex[]  = "0123456789abcdef";
    const uint8_t* text = (const uint8_t*)(in);
    for (int i = 0; i < (int)insize; i++) {
        out[i * 2]     = _hex[text[i] >> 4];
        out[i * 2 + 1] = _hex[text[i] & 0xf];
    }
}

void hex__unmarshal(const char* in, size_t insize, char* out) {
    if (insize & 1) {
        throw std::runtime_error("Invalid hex text size");
    }
    for (int i = 0; i < (int)insize; i += 2) {
        uint8_t hi, low;
        HEX(hi, in[i]);
        HEX(low, in[i + 1]);
        if (hi > 16 || low > 16) {
            fprintf(stderr, "hi:%d, lo:%d, %c\n", hi, low, in[i + 1]);
            throw std::runtime_error("Invalid hex text");
        }
        out[i / 2] = hi << 4 | low;
    }
}

}  // namespace unsimd

namespace {

struct EncodeUnit : hn::UnrollerUnit<EncodeUnit, u8, u8> {
    using D = hn::ScalableTag<u8>;
    const vu8 _f             = hn::Set(_du8, 0xF);
    const vu8 _hex_lut       = hn::LoadDup128(_du8, (const u8*)"0123456789abcdef");

    u8* _dest;
    EncodeUnit(u8* dest) : _dest(dest) {}

    vu8 Func(const ptrdiff_t idx, const vu8 x, const vu8) {
        auto higher_nibble = hn::ShiftRightSame(x, 4);
        auto lower_nibble  = hn::And(x, _f);
        auto hi            = hn::TableLookupBytes(_hex_lut, higher_nibble);
        auto lo            = hn::TableLookupBytes(_hex_lut, lower_nibble);
        hn::StoreInterleaved2(hi, lo, _du8, _dest + idx * 2);
        return hn::Zero(_du8);
    }

    bool StoreAndShortCircuitImpl(const ptrdiff_t idx, u8* to, const vu8 x) { return true; }
};

struct DecodeUnit : hn::UnrollerUnit2D<DecodeUnit, u8, u8, u8> {
    using D = hn::ScalableTag<u8>;
    const vu8 _f             = hn::Set(_du8, 0xF);
    // clang-format off
    const vu8 _hex_lut = hn::Dup128VecFromValues(_du8,
        /* 0 */ 0x10,        /* 1 */ 0x00,        /* 2 */ 0x00,        /* 3 */ 0x00 - 0x30,
        /* 4 */ 0x0A - 0x41, /* 5 */ 0x00,        /* 6 */ 0x0A - 0x61, /* 7 */ 0x00,
        /* 8 */ 0x00,        /* 9 */ 0x00,        /* a */ 0x00,        /* b */ 0x00,
        /* c */ 0x00,        /* d */ 0x00,        /* e */ 0x00,        /* f */ 0x00);
    // clang-format on
    vu8 _x0 = hn::Zero(_du8);
    vu8 _x1 = hn::Zero(_du8);

    inline vu8 lookup_pshufb(const vu8& xx) {
        const auto higher_nibble = hn::ShiftRightSame(xx, 4);
        const auto result        = hn::Add(xx, hn::TableLookupBytes(_hex_lut, higher_nibble));
        auto idx                 = hn::FindFirstTrue(_du8, hn::Gt(result, _f));
        if (HWY_UNLIKELY(idx != -1)) {
            throw lc::input_error(idx, 0);
        }
        return result;
    }

    vu8 Func(const ptrdiff_t idx, const vu8 x0, const vu8 x1, const vu8) {
        vu8 xx0 = hn::Zero(_du8);
        vu8 xx1 = hn::Zero(_du8);
        try {
            xx0 = lookup_pshufb(x0);
        } catch (const lc::input_error& e) {
            const auto shift = e.offset();
            throw lc::input_error(idx * 2 + shift * 2, hn::ExtractLane(x0, shift));
        }
        try {
            xx1 = lookup_pshufb(x1);
        } catch (const lc::input_error& e) {
            const auto shift = e.offset();
            throw lc::input_error(idx * 2 + shift * 2 + 1, hn::ExtractLane(x1, shift));
        }
        return hn::Or(hn::ShiftLeftSame(xx0, 4), xx1);
    }

    vu8 Load0Impl(const ptrdiff_t idx, const u8* from) {
        hn::LoadInterleaved2(_du8, from + idx * 2, _x0, _x1);
        return _x0;
    }

    vu8 Load1Impl(const ptrdiff_t idx, const u8* from) { return _x1; }
};

}  // namespace

namespace lc {

std::string hex_encode(const char* in, size_t len) {
    static constexpr HWY_FULL(u8) _du8{};
    static constexpr size_t N8 = hn::Lanes(_du8);
    size_t mod                 = len % N8;
    std::string result(2 * len, '\0');
    if (len > mod) {
        EncodeUnit unit((u8*)result.data());
        hn::Unroller(unit, (u8*)(const_cast<char*>(in)), (u8*)result.data(), len - mod);
    }
    if (mod > 0) {
        int start = len - mod;
        unsimd::hex__marshal(in + start, mod, &result[start * 2]);
    }
    return result;
}

std::string hex_decode(const char* in, size_t len) {
    static constexpr HWY_FULL(u8) _du8{};
    static constexpr size_t N8 = hn::Lanes(_du8);
    if (HWY_UNLIKELY(len & 1)) {
        throw std::runtime_error("Invalid hex text size");
    }

    size_t olen = len / 2;
    auto mod = olen % N8;
    std::string result(olen, '\0');
    if (olen > mod) {
        DecodeUnit unit;
        hn::Unroller(unit, (u8*)(const_cast<char*>(in)), (u8*)(const_cast<char*>(in)),
                     (u8*)result.data(), olen - mod);
    }
    if (mod > 0) {
        int start = olen - mod;
        unsimd::hex__unmarshal(in + start * 2, mod * 2, &result[start]);
    }
    return result;
}

}  // namespace lc
