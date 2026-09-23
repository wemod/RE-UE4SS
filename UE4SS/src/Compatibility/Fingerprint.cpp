#include <Compatibility/Fingerprint.hpp>
#include <array>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <Windows.h>
#include <bcrypt.h>

namespace RC::Compatibility
{
    auto executable_sha256(const std::filesystem::path& path) -> std::string
    {
        struct Algorithm
        {
            BCRYPT_ALG_HANDLE handle{};
            ~Algorithm()
            {
                if (handle) BCryptCloseAlgorithmProvider(handle, 0);
            }
        } algorithm;
        struct Hash
        {
            BCRYPT_HASH_HANDLE handle{};
            ~Hash()
            {
                if (handle) BCryptDestroyHash(handle);
            }
        } hash;
        auto check = [](NTSTATUS status) {
            if (status < 0) throw std::runtime_error{"Could not fingerprint the game executable"};
        };
        check(BCryptOpenAlgorithmProvider(&algorithm.handle, BCRYPT_SHA256_ALGORITHM, nullptr, 0));
        check(BCryptCreateHash(algorithm.handle, &hash.handle, nullptr, 0, nullptr, 0, 0));
        std::ifstream input{path, std::ios::binary};
        if (!input) throw std::runtime_error{"Could not open the game executable for fingerprinting"};
        std::array<unsigned char, 65536> buffer{};
        while (input)
        {
            input.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
            check(BCryptHashData(hash.handle, buffer.data(), static_cast<ULONG>(input.gcount()), 0));
        }
        if (!input.eof()) throw std::runtime_error{"Could not read the game executable for fingerprinting"};
        std::array<unsigned char, 32> digest{};
        check(BCryptFinishHash(hash.handle, digest.data(), static_cast<ULONG>(digest.size()), 0));
        constexpr std::string_view hex = "0123456789abcdef";
        std::string result;
        result.reserve(64);
        for (auto byte : digest)
        {
            result += hex[byte >> 4];
            result += hex[byte & 15];
        }
        return result;
    }
} // namespace RC::Compatibility
