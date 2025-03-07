#pragma once

#include <iomanip>
#include <sstream>

#include <QuasarEngine/Crypto/CryptoManager.h>

#include <QuasarEngine/Crypto/SHA256.h>

class Hasher {
public:
    static std::string sha256(const std::string& input) {
        return CryptoManager::instance().SHA256_Hash(input);
    }
};