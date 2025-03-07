#include "CryptoManager.h"

#include <stdexcept>
#include <cstring>
#include <iostream>
#include <iomanip>

CryptoManager::CryptoManager()
{
    
}

CryptoManager::~CryptoManager()
{
    
}

void CryptoManager::Init_SHA256()
{
    sha256 = std::make_unique<SHA256>();
}

void CryptoManager::Init_AES(const std::array<uint8_t, 16>& key)
{
    aes = std::make_unique<AES>(key);
}

std::string CryptoManager::SHA256_Hash(const std::string& input)
{
    return sha256->hash(input);
}

std::vector<uint8_t> CryptoManager::AES_Encrypt(const std::vector<uint8_t>& plaintext)
{
    std::vector<uint8_t> ciphertext;
    size_t length = plaintext.size();
    size_t blocks = (length + 15) / 16;

    for (size_t i = 0; i < blocks; ++i) {
        std::array<uint8_t, 16> block = { 0 };

        size_t start = i * 16;
        size_t end = std::min(start + 16, length);

        std::copy(plaintext.begin() + start, plaintext.begin() + end, block.begin());

        if (end - start < 16) {
            AddPadding(block, end - start);
        }

        auto encryptedBlock = aes->encrypt(block);
        ciphertext.insert(ciphertext.end(), encryptedBlock.begin(), encryptedBlock.end());
    }

    return ciphertext;
}

std::vector<uint8_t> CryptoManager::AES_Decrypt(const std::vector<uint8_t>& ciphertext)
{
    std::vector<uint8_t> plaintext;
    size_t length = ciphertext.size();
    size_t blocks = length / 16;

    for (size_t i = 0; i < blocks; ++i) {
        std::array<uint8_t, 16> block = { 0 };
        std::copy(ciphertext.begin() + i * 16, ciphertext.begin() + (i + 1) * 16, block.begin());

        auto decryptedBlock = aes->decrypt(block);
        plaintext.insert(plaintext.end(), decryptedBlock.begin(), decryptedBlock.end());
    }

    size_t padding = plaintext.back();
    plaintext.resize(plaintext.size() - padding);

    return plaintext;
}

void CryptoManager::PrintHex(const std::string& label, const std::vector<uint8_t>& data) {
    std::cout << label << ": ";
    for (uint8_t byte : data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    }
    std::cout << std::dec << std::endl;
}
