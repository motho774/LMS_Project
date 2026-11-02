#include "PasswordUtils.h"

#include <argon2.h>
#include <array>
#include <cstdint>
#include <random>
#include <string>

namespace {
    constexpr std::uint32_t TIME_COST = 2;
    constexpr std::uint32_t MEMORY_COST = 19456; // KiB
    constexpr std::uint32_t PARALLELISM = 1;
    constexpr std::size_t SALT_SIZE = 16;
    constexpr std::size_t HASH_SIZE = 32;

    std::string toHex(const unsigned char* data, std::size_t length) {
        const char digits[] = "0123456789abcdef";
        std::string result;

        for (std::size_t i = 0; i < length; ++i) {
            result += digits[data[i] >> 4];
            result += digits[data[i] & 0x0F];
        }

        return result;
    }

    bool fromHex(
        const std::string& hex,
        unsigned char* output,
        std::size_t outputSize
    ) {
        if (hex.size() != outputSize * 2) {
            return false;
        }

        auto hexValue = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };

        for (std::size_t i = 0; i < outputSize; ++i) {
            int high = hexValue(hex[2 * i]);
            int low = hexValue(hex[2 * i + 1]);

            if (high < 0 || low < 0) {
                return false;
            }

            output[i] = static_cast<unsigned char>((high << 4) | low);
        }

        return true;
    }
}

bool PasswordUtils::hashPassword(
    const std::string& password,
    std::string& hash,
    std::string& salt
) {
    if (password.empty()) {
        return false;
    }

    // Generate a new salt for this password.
    std::array<unsigned char, SALT_SIZE> saltBytes{};
    std::random_device random;

    try {
        for (auto& byte : saltBytes) {
            byte = static_cast<unsigned char>(random() & 0xFF);
        }
    } catch (...) {
        return false;
    }

    std::array<unsigned char, HASH_SIZE> hashBytes{};

    int result = argon2id_hash_raw(
        TIME_COST,
        MEMORY_COST,
        PARALLELISM,
        password.data(),
        password.size(),
        saltBytes.data(),
        saltBytes.size(),
        hashBytes.data(),
        hashBytes.size()
    );

    if (result != ARGON2_OK) {
        return false;
    }

    salt = toHex(saltBytes.data(), saltBytes.size());
    hash = toHex(hashBytes.data(), hashBytes.size());

    return true;
}

bool PasswordUtils::verifyPassword(
    const std::string& password,
    const std::string& hash,
    const std::string& salt
) {
    if (password.empty()) {
        return false;
    }

    std::array<unsigned char, SALT_SIZE> saltBytes{};
    std::array<unsigned char, HASH_SIZE> expectedHash{};
    std::array<unsigned char, HASH_SIZE> actualHash{};

    if (!fromHex(salt, saltBytes.data(), saltBytes.size()) ||
        !fromHex(hash, expectedHash.data(), expectedHash.size())) {
        return false;
    }

    int result = argon2id_hash_raw(
        TIME_COST,
        MEMORY_COST,
        PARALLELISM,
        password.data(),
        password.size(),
        saltBytes.data(),
        saltBytes.size(),
        actualHash.data(),
        actualHash.size()
    );

    if (result != ARGON2_OK) {
        return false;
    }

    // Compare all bytes rather than stopping at the first mismatch.
    unsigned char difference = 0;

    for (std::size_t i = 0; i < HASH_SIZE; ++i) {
        difference |= expectedHash[i] ^ actualHash[i];
    }

    return difference == 0;
}