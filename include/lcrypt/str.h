#pragma once

#include <tuple>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <lcrypt/base.h>

namespace lc {

std::vector<std::string_view>  //
str_split(std::string_view str, std::string_view delimiter, bool trim = false);

std::string str_join(const std::vector<std::string_view>& vs, std::string_view delimiter);

std::string str_toupper(std::string_view s);
std::string str_tolower(std::string_view s);

std::string_view str_trim(std::string_view str);

bool str_starts_with(std::string_view s, std::string_view perfix);
bool str_ends_with(std::string_view s, std::string_view perfix);

namespace detail {

enum KOption {
    Kint = 0,   /* signed integers */
    Kuint,      /* unsigned integers */
    Kfloat,     /* single-precision floating-point numbers */
    Kdouble,    /* double-precision floating-point numbers */
    Kchar,      /* fixed-length strings */
    Kstring,    /* strings with prefixed length */
    Kzstr,      /* zero-terminated strings */
    Kpadding,   /* padding */
    Kpaddalign, /* padding for alignment */
    Knop,       /* no-op (configuration or spaces) */
    Kend,
};

class PackFmtParser {
    const std::string_view fmt_;
    const int count_;
    int offset_;
    int islittle_;
    int maxalign_;
    size_t totalsize_;

    inline static constexpr char PACKPADBYTE = 0x00;
    static std::string to_string(KOption op);
    static std::exception_ptr make_error(std::string_view tname, KOption op);

public:
    using buffer_t = std::vector<char>;

    struct option_t {
        KOption op;
        size_t size;
        size_t ntoalign;
        int islittle;
    };

public:
    PackFmtParser(std::string_view fmt);

    void add_size(size_t);
    void nextop(option_t& option);

    void packpre(buffer_t& b, const option_t& op);
    void packone(buffer_t& b, const option_t& op);
    void packone(buffer_t& b, const option_t& op, std::string_view v);
    template <typename T, typename U = std::decay_t<T>>
    std::enable_if_t<std::is_integral_v<U> || std::is_floating_point_v<U>, void>  //
    packone(buffer_t& b, const option_t& op, T&& v) {
        switch (op.op) {
        case Kint: packint(b, (uint64_t)v, op, (v < 0)); break;
        case Kuint: packint(b, (uint64_t)v, op, 0); break;
        case Kfloat: {
            float f = std::forward<T>(v);
            copywithendian(b, (char*)&f, sizeof(f), op.islittle);
            break;
        }
        case Kdouble: {
            double f = std::forward<T>(v);
            copywithendian(b, (char*)&f, sizeof(f), op.islittle);
            break;
        }
        default: std::rethrow_exception(make_error(typeid(U).name(), op.op)); break;
        }
    }

    // unpackone()
    template <typename T, typename U = std::decay_t<T>>
    std::enable_if_t<std::is_integral_v<U> || std::is_floating_point_v<U>, void>  //
    unpackone(T& v, int& consumed, std::string_view s, const option_t& op) {
        switch (op.op) {
        case Kint:
        case Kuint: {
            auto res = unpackint(s.substr(0, op.size), islittle_, (op.op == Kint));
            v        = static_cast<U>(res);
            break;
        }
        case Kfloat: {
            buffer_t b;
            copywithendian(b, s.data(), op.size, op.islittle);
            v = static_cast<U>(*(float*)b.data());
            break;
        }
        case Kdouble: {
            buffer_t b;
            copywithendian(b, s.data(), op.size, op.islittle);
            v = static_cast<U>(*(double*)b.data());
            break;
        }
        default: std::rethrow_exception(make_error(typeid(U).name(), op.op)); break;
        }

        consumed = op.size;
    }

    void unpackone(std::string& v, int& consumed, std::string_view s, const option_t& op);
    void unpackone(buffer_t& v, int& consumed, std::string_view s, const option_t& op) {
        std::string ss;
        unpackone(ss, consumed, s, op);
        v = buffer_t(ss.begin(), ss.end());
    }

private:
    int getnum_from_current(int df);

    // packint()
    static void packint(buffer_t& b, uint64_t n, const option_t& op, int neg);
    static int64_t unpackint(std::string_view s, int islittle, int issigend);
    // int64_t unpackint(const option_t& op, int issigend);
    static void copywithendian(buffer_t& dest, const char* src, int size, int islittle);
};

}  // namespace detail

template <typename... Args>
std::vector<char>  //
str_pack(std::string_view fmt, Args&&... args) {
    std::vector<char> result;
    detail::PackFmtParser::option_t op;
    detail::PackFmtParser pfp(fmt);
    const auto f = [&](auto&& a) {
        for (;;) {
            pfp.nextop(op);
            pfp.packpre(result, op);
            switch (op.op) {
            case detail::Kpadding: pfp.packone(result, op); break;
            case detail::Kpaddalign:
            case detail::Knop: break;
            case detail::Kend: return;
            default: pfp.packone(result, op, std::forward<decltype(a)>(a)); return;
            }
        }
    };
    ((f(std::forward<Args>(args))), ...);
    // tail
    bool end = false;
    for (;;) {
        pfp.nextop(op);
        pfp.packpre(result, op);
        switch (op.op) {
        case detail::Kpadding: pfp.packone(result, op); break;
        case detail::Kpaddalign:
        case detail::Knop: break;
        case detail::Kend: end = true; break;
        default: throw std::runtime_error("Need params!!!"); break;
        }
        if (end) break;
    }
    return result;
}

template <typename... Args>
std::tuple<std::decay_t<Args>..., int>  //
str_unpack(std::string_view fmt, std::string_view data) {
    using R    = std::tuple<std::decay_t<Args>...>;
    int len    = data.size();
    int offset = 0;
    R res;
    detail::PackFmtParser::option_t op;
    detail::PackFmtParser pfp(fmt);

    const auto check = [&] {
        if (offset > len) {
            throw std::runtime_error(std::string("Data overflow. offset = ")
                                     + std::to_string(offset) + ", len = " + std::to_string(len));
        }
    };

    const auto f = [&](auto& x) {
        for (;;) {
            pfp.nextop(op);
            offset += op.ntoalign; /* skip alignment */
            pfp.add_size(op.ntoalign);
            switch (op.op) {
            case detail::Kpadding: offset += 1; pfp.add_size(op.ntoalign);  // fallthroungh
            case detail::Kpaddalign:
            case detail::Knop: break;
            default: {
                int consumed = 0;
                pfp.unpackone(x, consumed, data.substr(offset), op);
                offset += consumed;
                pfp.add_size(consumed);
                check();
                return;
            }
            }
            check();
        }
    };
    std::apply([&](auto&... xs) { ((f(xs)), ...); }, res);
    // tail
    bool end = false;
    for (;;) {
        pfp.nextop(op);
        offset += op.ntoalign;
        pfp.add_size(op.ntoalign);
        switch (op.op) {
        case detail::Kpadding: offset += 1; pfp.add_size(op.ntoalign);  // fallthroungh
        case detail::Kpaddalign:
        case detail::Knop: break;
        case detail::Kend: end = true; break;
        default: throw std::runtime_error("Need params!!!"); break;
        }
        check();
        if (end) break;
    }
    return std::tuple_cat(std::move(res), std::make_tuple(offset));
}

template <typename... Args>
std::tuple<std::decay_t<Args>..., int>  //
str_unpack(std::string_view fmt, const std::vector<char>& data) {
    return str_unpack<Args...>(fmt, std::string_view(data.data(), data.size()));
}

}  // namespace lc
