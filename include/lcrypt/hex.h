#pragma once

#include <string>
#include <string_view>
#include <lcrypt/base.h>

namespace lc {

std::string hex_encode(const char* buf, size_t len);
std::string hex_decode(const char* buf, size_t len);

template <typename V>
std::string hex_encode(const V& v) {
    auto s = to_span(v);
    return hex_encode(s.data(), s.size());
}

template <typename V>
std::string hex_decode(const V& v) {
    auto s = to_span(v);
    return hex_decode(s.data(), s.size());
}

}  // namespace lc
