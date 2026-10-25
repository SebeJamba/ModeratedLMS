#ifndef SECURITY_HPP
#define SECURITY_HPP

#include <string>

std::string generateSalt();
std::string hashPassword(const std::string& password, const std::string& salt);
bool verifyPassword(const std::string& password, const std::string& storedHash);

#endif // SECURITY_HPP