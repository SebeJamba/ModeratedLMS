#include "Security.hpp"
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#endif

std::string generateSalt() {
#ifdef _WIN32
    unsigned char saltBytes[16];
    if (BCryptGenRandom(nullptr, saltBytes, sizeof(saltBytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0) {
        std::ostringstream oss;
        for (int i = 0; i < 16; ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)saltBytes[i];
        }
        return oss.str();
    }
#endif
    return "default_salt_123";
}

std::string hashPassword(const std::string& password, const std::string& salt) {
    std::string combined = salt + ":" + password;
    unsigned long hash = 5381;
    for (char c : combined) {
        hash = ((hash << 5) + hash) + c;
    }
    std::ostringstream oss;
    oss << salt << "$" << std::hex << hash;
    return oss.str();
}

bool verifyPassword(const std::string& password, const std::string& storedHash) {
    size_t pos = storedHash.find('$');
    if (pos == std::string::npos) return false;
    std::string salt = storedHash.substr(0, pos);
    return hashPassword(password, salt) == storedHash;
}