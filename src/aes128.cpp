#include "lcrypt/aes128.h"
#include <array>
#include <exception>
#include <stdexcept>
#include <hwy/highway.h>
#include <stdint.h>
#include <string.h>

namespace hn = hwy::HWY_NAMESPACE;

struct aes128 {
    using vec_t  = hn::Vec<HWY_FULL(uint8_t)>;
    using keys_t = std::array<vec_t, 20>;

    inline static HWY_FULL(uint8_t) _d8;
    inline static HWY_FULL(uint32_t) _d32;
    inline static constexpr size_t N8 = hn::Lanes(_d8);

    static keys_t load_key(std::string_view key) {
        uint8_t keyb[16] = {0};
        hwy::ZeroBytes(keyb, 16);
        hwy::CopyBytes(key.data(), keyb, HWY_MIN(key.size(), 16));
        keys_t key_schedule;
        key_schedule[0]  = hn::LoadDup128(_d8, keyb);
        key_schedule[1]  = key_expansion<0x01>(key_schedule[0]);
        key_schedule[2]  = key_expansion<0x02>(key_schedule[1]);
        key_schedule[3]  = key_expansion<0x04>(key_schedule[2]);
        key_schedule[4]  = key_expansion<0x08>(key_schedule[3]);
        key_schedule[5]  = key_expansion<0x10>(key_schedule[4]);
        key_schedule[6]  = key_expansion<0x20>(key_schedule[5]);
        key_schedule[7]  = key_expansion<0x40>(key_schedule[6]);
        key_schedule[8]  = key_expansion<0x80>(key_schedule[7]);
        key_schedule[9]  = key_expansion<0x1B>(key_schedule[8]);
        key_schedule[10] = key_expansion<0x36>(key_schedule[9]);

        // generate decryption keys in reverse order.
        // k[10] is shared by last encryption and first decryption rounds
        // k[0] is shared by first encryption round and last decryption round (and is the original user key)
        // For some implementation reasons, decryption key schedule is NOT the encryption key schedule in reverse order
        key_schedule[11] = hn::AESInvMixColumns(key_schedule[9]);
        key_schedule[12] = hn::AESInvMixColumns(key_schedule[8]);
        key_schedule[13] = hn::AESInvMixColumns(key_schedule[7]);
        key_schedule[14] = hn::AESInvMixColumns(key_schedule[6]);
        key_schedule[15] = hn::AESInvMixColumns(key_schedule[5]);
        key_schedule[16] = hn::AESInvMixColumns(key_schedule[4]);
        key_schedule[17] = hn::AESInvMixColumns(key_schedule[3]);
        key_schedule[18] = hn::AESInvMixColumns(key_schedule[2]);
        key_schedule[19] = hn::AESInvMixColumns(key_schedule[1]);
        return key_schedule;
    }

    static std::vector<uint8_t>  //
    encrypt(std::string_view plain, const keys_t& key_schedule) {
        size_t len        = plain.size();
        size_t padding    = 16 - (len % 16);
        char tail_buf[64] = {0};
        std::vector<uint8_t> result(len + padding);
        size_t idx         = 0;
        const uint8_t* src = reinterpret_cast<const uint8_t*>(plain.data());
        uint8_t* dest      = reinterpret_cast<uint8_t*>(result.data());

        static constexpr auto enc_blk = [](auto in, const auto& key_schedule) {
            in = hn::Xor(in, key_schedule[0]);
            in = hn::AESRound(in, key_schedule[1]);
            in = hn::AESRound(in, key_schedule[2]);
            in = hn::AESRound(in, key_schedule[3]);
            in = hn::AESRound(in, key_schedule[4]);
            in = hn::AESRound(in, key_schedule[5]);
            in = hn::AESRound(in, key_schedule[6]);
            in = hn::AESRound(in, key_schedule[7]);
            in = hn::AESRound(in, key_schedule[8]);
            in = hn::AESRound(in, key_schedule[9]);
            in = hn::AESLastRound(in, key_schedule[10]);
            return in;
        };

        while (idx + N8 - 1 < len) {
            auto in = hn::LoadU(_d8, src + idx);
            in      = enc_blk(in, key_schedule);
            hn::StoreU(in, _d8, dest + idx);
            idx += N8;
        }

        if (HWY_LIKELY(idx != len)) {
            const size_t remaining = len - idx;
            memset(tail_buf, (char)padding, 64);
            hwy::CopyBytes(src + idx, tail_buf, remaining);
            auto in = hn::LoadU(_d8, (const uint8_t*)tail_buf);
            in      = enc_blk(in, key_schedule);
            hn::StoreN(in, _d8, dest + idx, remaining + padding);
        }

        return result;
    }

    static inline std::vector<uint8_t>  //
    encrypt(std::string_view plain, std::string_view key) {
        return encrypt(plain, load_key(key));
    }

    static std::vector<uint8_t>  //
    decrypt(std::string_view cipher, const keys_t& key_schedule) {
        size_t len = cipher.size();
        if (HWY_UNLIKELY(len % 16 != 0)) {
            throw std::runtime_error("Invalid aes128 size");
        }
        size_t idx = 0;
        std::vector<uint8_t> result(len);
        uint8_t* dest      = reinterpret_cast<uint8_t*>(&result[0]);
        const uint8_t* src = reinterpret_cast<const uint8_t*>(cipher.data());

        static constexpr auto dec_blk = [](vec_t in, const keys_t& key_schedule) {
            in = hn::Xor(in, key_schedule[10]);
            in = hn::AESRoundInv(in, key_schedule[11]);
            in = hn::AESRoundInv(in, key_schedule[12]);
            in = hn::AESRoundInv(in, key_schedule[13]);
            in = hn::AESRoundInv(in, key_schedule[14]);
            in = hn::AESRoundInv(in, key_schedule[15]);
            in = hn::AESRoundInv(in, key_schedule[16]);
            in = hn::AESRoundInv(in, key_schedule[17]);
            in = hn::AESRoundInv(in, key_schedule[18]);
            in = hn::AESRoundInv(in, key_schedule[19]);
            return hn::AESLastRoundInv(in, key_schedule[0]);
        };

        while (idx + N8 - 1 < len) {
            auto in = hn::LoadU(_d8, src + idx);
            in      = dec_blk(in, key_schedule);
            hn::StoreU(in, _d8, dest + idx);
            idx += N8;
        }

        if (idx != len) {
            const size_t remaining = len - idx;
            auto in                = hn::LoadN(_d8, src + idx, remaining);
            in                     = dec_blk(in, key_schedule);
            hn::StoreN(in, _d8, dest + idx, remaining);
        }

        auto padding = (int)result[len - 1];
        result.resize(len - padding);
        return result;
    }

    static std::vector<uint8_t>  //
    decrypt(std::string_view cipher, std::string_view key) {
        return decrypt(cipher, load_key(key));
    }

private:
    template <uint8_t Rcon>
    static vec_t key_expansion(vec_t key) {
        auto keygened = hn::AESKeyGenAssist<Rcon>(key);
        keygened      = hn::BitCast(_d8, hn::Broadcast<3>(hn::BitCast(_d32, keygened)));
        key           = hn::Xor(key, hn::ShiftLeftBytes<4>(key));
        key           = hn::Xor(key, hn::ShiftLeftBytes<4>(key));
        key           = hn::Xor(key, hn::ShiftLeftBytes<4>(key));
        auto r        = hn::Xor(key, keygened);
        return r;
    }
};

namespace lc {

std::vector<uint8_t>
aes128_enc(const char* plain, size_t plain_size, const char* key, size_t key_size) {
    return aes128::encrypt(std::string_view(plain, plain_size), std::string_view(key, key_size));
}

std::vector<uint8_t>
aes128_dec(const char* cipher, size_t cipher_size, const char* key, size_t key_size) {
    return aes128::decrypt(std::string_view(cipher, cipher_size), std::string_view(key, key_size));
}

}  // namespace lc
