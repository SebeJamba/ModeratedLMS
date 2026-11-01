#include "Security.hpp"
#include <array>
#include <windows.h>
#include <bcrypt.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
constexpr unsigned long long iterations = 600000;
std::string hex(const unsigned char *bytes, size_t count) {
    std::ostringstream out;
    for (size_t i = 0; i < count; ++i)
        out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
    return out.str();
}
std::array<unsigned char, 16> saltBytes(const std::string &salt) {
    if (salt.size() != 32)
        throw std::invalid_argument("Salt must contain 16 random bytes in hexadecimal");
    std::array<unsigned char, 16> bytes{};
    for (size_t i = 0; i < bytes.size(); ++i) {
        auto part = salt.substr(2 * i, 2);
        if (part.find_first_not_of("0123456789abcdef") != std::string::npos)
            throw std::invalid_argument("Invalid salt");
        bytes[i] = static_cast<unsigned char>(std::stoul(part, nullptr, 16));
    }
    return bytes;
}
bool equal(const std::string &a, const std::string &b) {
    if (a.size() != b.size())
        return false;
    unsigned int difference = 0;
    for (size_t i = 0; i < a.size(); ++i)
        difference |= static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]);
    return difference == 0;
}
} // namespace
std::string generateSalt() {
    std::array<unsigned char, 16> salt{};
    if (BCryptGenRandom(nullptr, salt.data(), static_cast<ULONG>(salt.size()),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
        throw std::runtime_error("Secure random generator failed");
    return hex(salt.data(), salt.size());
}
std::string hashPassword(const std::string &password, const std::string &salt) {
    auto bytes = saltBytes(salt);
    std::array<unsigned char, 32> derived{};
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr,
                                    BCRYPT_ALG_HANDLE_HMAC_FLAG) != 0)
        throw std::runtime_error("Cannot initialise PBKDF2 provider");
    auto status = BCryptDeriveKeyPBKDF2(
        algorithm, reinterpret_cast<PUCHAR>(const_cast<char *>(password.data())),
        static_cast<ULONG>(password.size()), bytes.data(), static_cast<ULONG>(bytes.size()),
        iterations, derived.data(), static_cast<ULONG>(derived.size()), 0);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (status != 0)
        throw std::runtime_error("Password derivation failed");
    return "pbkdf2$600000$" + salt + "$" + hex(derived.data(), derived.size());
}
bool verifyPassword(const std::string &password, const std::string &stored) {
    const std::string prefix = "pbkdf2$600000$";
    if (stored.rfind(prefix, 0) == 0) {
        if (stored.size() != prefix.size() + 32 + 1 + 64 || stored[prefix.size() + 32] != '$')
            return false;
        try {
            return equal(hashPassword(password, stored.substr(prefix.size(), 32)), stored);
        } catch (const std::invalid_argument &) {
            return false;
        }
    }
    // Old hashes are accepted only for login migration; new writes use PBKDF2.
    auto split = stored.find('$');
    if (split == std::string::npos || stored.find('$', split + 1) != std::string::npos)
        return false;
    auto salt = stored.substr(0, split);
    if (salt.size() != 32 && salt != "default_salt_123")
        return false;
    unsigned long legacy = 5381;
    for (char c : salt + ":" + password)
        legacy = ((legacy << 5) + legacy) + c;
    std::ostringstream old;
    old << salt << "$" << std::hex << legacy;
    return equal(old.str(), stored);
}
