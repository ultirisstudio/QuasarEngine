#pragma once

#include <vector>
#include <string>
#include <memory>

#include <QuasarEngine/Core/Singleton.h>

#include <QuasarEngine/Crypto/SHA256.h>
#include <QuasarEngine/Crypto/AES.h>

class CryptoManager : public Singleton<CryptoManager> {
public:
    CryptoManager();
    ~CryptoManager();

    void Init_SHA256();
    void Init_AES(const std::array<uint8_t, 16>& key);

    std::string SHA256_Hash(const std::string& input);

    std::vector<uint8_t> AES_Encrypt(const std::vector<uint8_t>& plaintext);
    std::vector<uint8_t> AES_Decrypt(const std::vector<uint8_t>& ciphertext);

    static void PrintHex(const std::string& label, const std::vector<uint8_t>& data);

private:
    std::unique_ptr<SHA256> sha256;
    std::unique_ptr<AES> aes;

    void AddPadding(std::array<uint8_t, 16>& block, size_t length) {
        size_t padding = 16 - length;
        for (size_t i = length; i < 16; ++i) {
            block[i] = static_cast<uint8_t>(padding);
        }
    }
};
