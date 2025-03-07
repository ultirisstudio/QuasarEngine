#pragma once

#include <iomanip>
#include <sstream>

#include <openssl/sha.h>

class Hasher {
public:
    static std::string sha256(const std::string& input) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256((unsigned char*)input.c_str(), input.size(), hash);
        std::stringstream ss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
        }
        return ss.str();
    }
};