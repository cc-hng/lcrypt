#include <stdexcept>
#include <hwy/contrib/unroller/unroller-inl.h>
#include <hwy/highway.h>
#include <lcrypt/str.h>
#include <string.h>

namespace hn = hwy::HWY_NAMESPACE;

static HWY_FULL(uint8_t) _du8;
using vec8_t = hn::Vec<decltype(_du8)>;

#include <hwy/print-inl.h>
#define PRINT_VEC8(caption, v8) hn::Print(_du8, caption, v8, 0, hn::Lanes(_du8))

namespace lc {

namespace detail {

struct UpperUnit : hn::UnrollerUnit<UpperUnit, uint8_t, uint8_t> {
    using TT = hn::ScalableTag<uint8_t>;
    inline static TT _d;
    inline static hn::Vec<TT> _0x61 = hn::Set(_d, 0x61);
    inline static hn::Vec<TT> _0x7a = hn::Set(_d, 0x7a);
    inline static hn::Vec<TT> _32   = hn::Set(_d, 32);

    hn::Vec<TT> Func(ptrdiff_t idx, const hn::Vec<TT> xx, const hn::Vec<TT> yy) {
        (void)idx;
        (void)yy;
        auto m = hn::And(hn::Ge(xx, _0x61), hn::Le(xx, _0x7a));
        return hn::MaskedSubOr(xx, m, xx, _32);
    }
};

struct LowerUnit : hn::UnrollerUnit<LowerUnit, uint8_t, uint8_t> {
    using TT = hn::ScalableTag<uint8_t>;
    inline static TT _d;
    inline static hn::Vec<TT> _0x41 = hn::Set(_d, 0x41);
    inline static hn::Vec<TT> _0x5a = hn::Set(_d, 0x5a);
    inline static hn::Vec<TT> _32   = hn::Set(_d, 32);

    hn::Vec<TT> Func(ptrdiff_t idx, const hn::Vec<TT> xx, const hn::Vec<TT> yy) {
        (void)idx;
        (void)yy;
        auto m = hn::And(hn::Ge(xx, _0x41), hn::Le(xx, _0x5a));
        return hn::MaskedAddOr(xx, m, xx, _32);
    }
};

}  // namespace detail

std::string str_toupper(std::string_view s) {
    size_t len = s.size();
    detail::UpperUnit upperfn;
    std::string out(len, '\0');
    hn::Unroller(upperfn, (uint8_t*)s.data(), (uint8_t*)out.data(), len);
    return out;
}

std::string str_tolower(std::string_view s) {
    size_t len = s.size();
    detail::LowerUnit lowerfn;
    std::string out(len, '\0');
    hn::Unroller(lowerfn, (uint8_t*)s.data(), (uint8_t*)out.data(), len);
    return out;
}

std::vector<std::string_view>  //
str_split(std::string_view str, std::string_view delimiter, bool trim) {
    size_t start = 0;
    size_t end   = 0;
    size_t dlen  = delimiter.size();
    std::vector<std::string_view> result;
    result.reserve(16);

    while ((end = str.find(delimiter, start)) != std::string_view::npos) {
        auto s = str.substr(start, end - start);
        result.emplace_back(trim ? str_trim(s) : s);
        start = end + dlen;
    }
    auto s = str.substr(start);
    result.emplace_back(trim ? str_trim(s) : s);
    return result;
}

std::string str_join(const std::vector<std::string_view>& vs, std::string_view delimiter) {
    int count      = 0;
    size_t dlen    = delimiter.size();
    const char* pd = delimiter.data();
    for (const auto& s : vs) {
        count += s.size();
    }
    count += dlen * (vs.size() - 1);

    std::string out(count, '\0');
    char* pout = out.data();
    bool first = true;
    for (const auto& s : vs) {
        if (!first) {
            memcpy(pout, pd, dlen);
            pout += dlen;
        }
        first      = false;
        size_t len = s.size();
        memcpy(pout, s.data(), len);
        pout += len;
    }
    return out;
}

std::string_view str_trim(std::string_view str) {
    size_t start = 0;
    while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start]))) {
        ++start;
    }

    size_t end = str.size();
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }

    return str.substr(start, end - start);
}

bool str_starts_with(std::string_view s, std::string_view prefix) {
    const size_t plen = prefix.size();
    return s.size() >= plen && memcmp(s.data(), prefix.data(), plen) == 0;
}

bool str_ends_with(std::string_view s, std::string_view prefix) {
    const size_t slen = s.size();
    const size_t plen = prefix.size();
    const char* ps    = s.data();
    return slen >= plen && memcmp(ps + slen - plen, prefix.data(), plen) == 0;
}

}  // namespace lc
