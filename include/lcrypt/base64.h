#pragma once

#include <string>
#include <string_view>
#include <lcrypt/base.h>

namespace lc {

std::string base64_encode(const char* buf, size_t len);
std::string base64_decode(const char* buf, size_t len);

inline size_t base64_encode_size(const char* buf, size_t len) {
    return ((len + 2) / 3) * 4;
}

inline size_t base64_decode_size(const char* buf, size_t len) {
    size_t padding = 0;
    for (int i = len - 1; i >= 0 && buf[i] == '='; --i, ++padding)
        ;
    return (len / 4) * 3 - padding;
}

template <typename V>
std::string base64_encode(const V& v) {
    auto s = to_span(v);
    return base64_encode(s.data(), s.size());
}

template <typename V>
std::string base64_decode(const V& v) {
    auto s = to_span(v);
    return base64_decode(s.data(), s.size());
}

template <typename V>
size_t base64_encode_size(const V& v) {
    auto s = to_span(v);
    return base64_encode_size(s.data(), s.size());
}

template <typename V>
size_t base64_decode_size(const V& v) {
    auto s = to_span(v);
    return base64_decode_size(s.data(), s.size());
}

}  // namespace lc
