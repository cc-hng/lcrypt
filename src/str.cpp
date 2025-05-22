#include <algorithm>
#include <stdexcept>
#include <tuple>
#include <hwy/contrib/unroller/unroller-inl.h>
#include <hwy/highway.h>
#include <lcrypt/base.h>
#include <lcrypt/str.h>
#include <limits.h>
#include <string.h>

namespace hn = hwy::HWY_NAMESPACE;

static HWY_FULL(uint8_t) _du8;
using vec8_t = hn::Vec<decltype(_du8)>;

#define MAX_SIZET ((size_t)(~(size_t)0))

#define MAXSIZE (sizeof(size_t) < sizeof(int) ? MAX_SIZET : (size_t)(INT_MAX))

/* number of bits in a character */
#define NB 8

/* mask for one character (NB 1's) */
#define MC ((1 << NB) - 1)

#define SZINT ((int)sizeof(uint64_t))

namespace lc {

namespace detail {

inline std::string str_toupper0(std::string_view str) {
    std::string result(str);
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

inline std::string str_tolower0(std::string_view str) {
    std::string result(str);
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

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

inline int mcmp(const void* s1, const void* s2, size_t n) {
#if HWY_COMPILER_MSVC
    return memcmp(s1, s2, n);
#else
    return __builtin_memcmp(s1, s2, n);
#endif
}

}  // namespace detail

namespace detail {

static inline int isdigit(int c) {
    return '0' <= c && c <= '9';
}

PackFmtParser::PackFmtParser(std::string_view fmt)
  : fmt_(fmt)
  , count_(fmt.size())
  , offset_(0)
  , islittle_(HWY_IS_LITTLE_ENDIAN)
  , maxalign_(1)
  , totalsize_(0) {
}

void PackFmtParser::add_size(size_t s) {
    totalsize_ += s;
}

void PackFmtParser::nextop(option_t& op) {
    // clang-format off
    struct cD { char c; union { int i; double u; void *s; } u; };
    // clang-format on
    op.islittle = islittle_;
    op.ntoalign = 0;
    op.size     = 0;
    op.op       = Knop;

    int align = -1;
    if (offset_ < count_) {
        char ch = fmt_[offset_++];
        // clang-format off
        switch (ch) {
        case 'b': op.size = sizeof(char); op.op = Kint; break;
        case 'B': op.size = sizeof(char); op.op = Kuint; break;
        case 'h': op.size = sizeof(short); op.op = Kint; break;
        case 'H': op.size = sizeof(short); op.op = Kint; break;
        case 'l': op.size = sizeof(long); op.op = Kuint; break;
        case 'L': op.size = sizeof(long); op.op = Kuint; break;
        case 'T': op.size = sizeof(size_t); op.op = Kuint; break;
        case 'f': op.size = sizeof(float); op.op = Kfloat; break;
        case 'd': op.size = sizeof(double); op.op = Kdouble; break;
        case 'i': op.size = getnum_from_current(sizeof(int)); op.op = Kint; break;
        case 'I': op.size = getnum_from_current(sizeof(int)); op.op = Kuint; break;
        case 's': op.size = getnum_from_current(sizeof(size_t)); op.op = Kstring; break;
        case 'z': op.op = Kzstr; break;
        case 'x': op.size = 1; op.op = Kpadding; break;
        case 'c':
            op.size = getnum_from_current(-1);
            if (op.size == -1) throw std::runtime_error("missing size for format option 'c'");
            op.op = Kchar;
            break;
        case 'X': {
            struct option_t op0;
            nextop(op0);
            if (op0.op == Kchar || op0.size == 0) {
                throw std::runtime_error("invalid next option for option 'X'");
            }
            align = op0.size;
            op.op = Kpaddalign;
            break;
        }
        case ' ': break;
        case '<' : islittle_ = 1; break;
        case '>' : islittle_ = 0; break;
        case '=' : islittle_ = HWY_IS_LITTLE_ENDIAN; break;
        case '!': {
            const int maxalign = offsetof(struct cD, u);
            maxalign_ =   getnum_from_current(maxalign);
            break;
        }
        default: throw input_error(offset_-1, ch);
        };
        // clang-format on
    } else {
        op.op = Kend;
    }

    if (op.op != Kpaddalign) {
        align = op.size;
    }
    if (align <= 1 || op.op == Kchar) {
        op.ntoalign = 0;
    } else {
        if (align > maxalign_) { /* enforce maximum alignment */
            align = maxalign_;
        }
        if ((align & (align - 1)) != 0) {
            throw std::runtime_error("format asks for alignment not power of 2");
        }
        op.ntoalign = (align - (int)(totalsize_ & (align - 1))) & (align - 1);
    }
    // fprintf(stderr, "op: %d - %d - %d - %d - totalsize: %d\n", op.islittle, op.op, op.size,
    //         op.ntoalign, totalsize_);
}

int PackFmtParser::getnum_from_current(int df) {
    if (!isdigit(fmt_[offset_])) {
        return df;
    } else {
        int a = 0;
        do {
            a = a * 10 + (fmt_[offset_++] - '0');
        } while (isdigit(fmt_[offset_]) && a <= ((int)MAXSIZE - 9) / 10);
        return a;
    }
}

void PackFmtParser::packint(buffer_t& b, uint64_t n, const option_t& op, int neg) {
    auto islittle = op.islittle;
    auto size     = op.size;
    int i;
    b.resize(b.size() + size);
    char* buff                    = &b.back() + 1 - size;
    buff[islittle ? 0 : size - 1] = (char)(n & MC);
    for (i = 1; i < size; i++) {
        n >>= NB;
        buff[islittle ? i : size - 1 - i] = (char)(n & MC);
    }

    if (neg && size > SZINT) {
        for (i = SZINT; i < size; i++) {
            buff[islittle ? i : size - 1 - i] = (char)MC;
        }
    }
}

int64_t PackFmtParser::unpackint(std::string_view s, int islittle, int issigned) {
    uint64_t res = 0;
    int i;
    size_t size = s.size();
    int limit   = (size <= SZINT) ? size : SZINT;
    for (i = limit - 1; i >= 0; i--) {
        res <<= NB;
        res |= (uint64_t)(uint8_t)s[islittle ? i : size - 1 - i];
    }
    if (size < SZINT) { /* real size smaller than lua_Integer? */
        if (issigned) { /* needs sign extension? */
            uint64_t mask = (uint64_t)1 << (size * NB - 1);
            res           = ((res ^ mask) - mask); /* do sign extension */
        }
    } else if (size > SZINT) { /* must check unread bytes */
        int mask = (!issigned || (int64_t)res >= 0) ? 0 : MC;
        for (i = limit; i < size; i++) {
            if (HWY_UNLIKELY((uint8_t)s[islittle ? i : size - 1 - i] != mask))
                throw std::runtime_error(std::to_string(size)
                                         + "-byte integer does not fit into Integer");
        }
    }
    return (int64_t)res;
}

void PackFmtParser::copywithendian(buffer_t& b, const char* src, int size, int islittle) {
    if (islittle_ == HWY_IS_LITTLE_ENDIAN) {
        b.insert(b.end(), src, src + size);
    } else {
        b.resize(b.size() + size);
        auto it = b.rbegin();
        for (int i = 0; i < size; i++, ++it) {
            *it = src[i];
        }
    }
}

void PackFmtParser::packpre(buffer_t& b, const option_t& op) {
    int ntoalign = op.ntoalign;
    add_size(ntoalign + op.size);
    while (ntoalign-- > 0) {
        b.emplace_back(PACKPADBYTE);
    }
}

void PackFmtParser::packone(buffer_t& b, const option_t& op) {
    if (op.op != Kpadding) {
        std::rethrow_exception(make_error("padding", op.op));
    }
    b.emplace_back(PACKPADBYTE);
}

void PackFmtParser::packone(buffer_t& b, const option_t& op, std::string_view v) {
    switch (op.op) {
    case Kchar: { /* fixed-size string */
        std::string_view s = v;
        int padding        = op.size - s.size();
        if (padding < 0) {
            throw std::runtime_error("cn: string longer than given size");
        }
        b.insert(b.end(), v.begin(), v.end());
        if (padding > 0) {
            b.resize(b.size() + padding, PACKPADBYTE);
        }
        break;
    }
    case Kstring: { /* strings with length count */
        packint(b, (uint64_t)v.size(), op, 0);
        b.insert(b.end(), v.begin(), v.end());
        add_size(v.size());
        break;
    }
    case Kzstr: { /* zero-terminated string */
        b.insert(b.end(), v.begin(), v.end());
        b.emplace_back('\0');
        add_size(v.size() + 1);
        break;
    }
    default: std::rethrow_exception(make_error("string", op.op)); break;
    }
}

void PackFmtParser::unpackone(std::string& v, int& consumed, std::string_view s, const option_t& op) {
    switch (op.op) {
    case Kchar:
        v.append(s.data(), op.size);
        consumed = op.size;
        break;
    case Kstring: {
        auto len = (size_t)unpackint(s.substr(0, op.size), op.islittle, 0);
        v.append(s.data() + op.size, len);
        consumed = op.size + len;
        break;
    }
    case Kzstr: {
        auto len = strlen(s.data());
        v.append(s.data(), len);
        consumed = len + 1;
        break;
    }
    default: std::rethrow_exception(make_error("string", op.op)); break;
    }
}

std::string PackFmtParser::to_string(KOption op) {
    switch (op) {
    case Kint: return "int";
    case Kuint: return "uint";
    case Kfloat: return "float";
    case Kdouble: return "double";
    case Kchar: return "charn";
    case Kstring: return "string";
    case Kzstr: return "zstr";
    case Kpadding: return "padding";
    case Kpaddalign: return "paddalign";
    case Knop: return "nop";
    case Kend: return "end";
    }
    HWY_ASSERT(0);
}

std::exception_ptr PackFmtParser::make_error(std::string_view tname, KOption op) {
    char buf[64] = {0};
    auto n = snprintf(buf, 64, "Type dismatch(%s, %s)", std::string(tname).c_str(), to_string(op));
    return std::make_exception_ptr(std::runtime_error(std::string(buf, n)));
}

}  // namespace detail

std::string str_toupper(std::string_view s) {
    size_t len = s.size();
    if (len < 16) {
        return detail::str_toupper0(s);
    } else {
        detail::UpperUnit upperfn;
        std::string out(len, '\0');
        hn::Unroller(upperfn, (uint8_t*)s.data(), (uint8_t*)out.data(), len);
        return out;
    }
}

std::string str_tolower(std::string_view s) {
    size_t len = s.size();
    if (len < 16) {
        return detail::str_tolower0(s);
    } else {
        detail::LowerUnit lowerfn;
        std::string out(len, '\0');
        hn::Unroller(lowerfn, (uint8_t*)s.data(), (uint8_t*)out.data(), len);
        return out;
    }
}

std::vector<std::string_view>  //
str_split(std::string_view str, std::string_view delimiter, bool trim) {
#if LC_HAS_MEMMEM
    size_t start       = 0;
    size_t end         = 0;
    size_t dlen        = delimiter.size();
    size_t slen        = str.size();
    const char* ps     = str.data();
    const char* ps_end = ps + slen;
    const char* pd     = delimiter.data();
    std::vector<std::string_view> result;
    result.reserve(16);

    for (;;) {
        auto p = (const char*)memmem(ps, ps_end - ps, pd, dlen);
        if (!p) {
            result.emplace_back(std::string_view(ps, ps_end - ps));
            break;
        }
        result.emplace_back(std::string_view(ps, p - ps));
        ps = p + dlen;
    }

    return result;
#else
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
#endif
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
            hwy::CopyBytes(pd, pout, dlen);
            pout += dlen;
        }
        first      = false;
        size_t len = s.size();
        hwy::CopyBytes(s.data(), pout, len);
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
    return s.size() >= plen && detail::mcmp(s.data(), prefix.data(), plen) == 0;
}

bool str_ends_with(std::string_view s, std::string_view prefix) {
    const size_t slen = s.size();
    const size_t plen = prefix.size();
    const char* ps    = s.data();
    return slen >= plen && detail::mcmp(ps + slen - plen, prefix.data(), plen) == 0;
}

}  // namespace lc
