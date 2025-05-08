#pragma once

#include <vector>
#include <lcrypt/base.h>
#include <stdint.h>

namespace lc {

std::vector<uint8_t>
aes128_enc(const char* plain, size_t plain_size, const char* key, size_t key_size);

std::vector<uint8_t>
aes128_dec(const char* cipher, size_t cipher_size, const char* key, size_t key_size);

template <typename Tp, typename Tk>
std::vector<uint8_t>  //
aes128_enc(const Tp& plain, const Tk& key) {
    auto p = to_span(plain);
    auto k = to_span(key);
    return aes128_enc(p.data(), p.size(), k.data(), k.size());
}

template <typename Tp, typename Tk>
std::vector<uint8_t>  //
aes128_dec(const Tp& plain, const Tk& key) {
    auto p = to_span(plain);
    auto k = to_span(key);
    return aes128_dec(p.data(), p.size(), k.data(), k.size());
}

}  // namespace lc
