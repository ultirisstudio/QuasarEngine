#pragma once

#include <iostream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <cstdint>
#include <cstring>

class SHA256 {
public:
    SHA256() {
        m_h[0] = 0x6A09E667;
        m_h[1] = 0xBB67AE85;
        m_h[2] = 0x3C6EF372;
        m_h[3] = 0xA54FF53A;
        m_h[4] = 0x510E527F;
        m_h[5] = 0x9B05688C;
        m_h[6] = 0x1F83D9AB;
        m_h[7] = 0x5BE0CD19;
    }

    std::string hash(const std::string& input) {
        std::vector<uint8_t> paddedMessage = padMessage(input);

        for (size_t i = 0; i < paddedMessage.size(); i += 64) {
            processBlock(&paddedMessage[i]);
        }

        std::ostringstream result;
        for (int i = 0; i < 8; ++i) {
            result << std::hex << std::setw(8) << std::setfill('0') << m_h[i];
        }
        return result.str();
    }

private:
    uint32_t m_h[8];

    const uint32_t m_k[64] = {
        0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
        0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
        0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
        0x72BE5D74, 0x80B6F2A8, 0x9BDC06A7, 0xCF692694,
        0xF27B7A53, 0x4CC5D4B6, 0x597F299C, 0x5FCB6FAB,
        0x6C44198C, 0x4A475817, 0xFFFFBFC6, 0xFF5A4E6A,
        0x601D6B6C, 0x62B4D4C6, 0x97C9A0C4, 0x3D423906,
        0x23F6C3A0, 0xA6745358, 0x2B076D7E, 0x06F51849,
        0x3505E18C, 0x249D5E36, 0x1755671C, 0x4E6BB6C5,
        0x031D5054, 0x82A3A4C9, 0x220C0233, 0x8B84C4E9,
        0xE6F7E149, 0xC7D6C6F7, 0x8B8A9E34, 0xE4C5F1DC,
        0x87E0A3B0, 0x6A6A227C, 0xF6E8F65A, 0x4A057053,
        0xD177B25B, 0x1D70620B, 0x30F0B270, 0x2C29C4F9,
        0x45C0FF42, 0x0CB60F1D, 0x803C8B7C, 0x77C60B14,
        0xE0E85579, 0xD5BC99F6, 0x57F8C4B1, 0x2F59E20C
    };

    uint32_t rotateRight(uint32_t value, unsigned int bits) {
        return (value >> bits) | (value << (32 - bits));
    }

    std::vector<uint8_t> padMessage(const std::string& input) {
        std::vector<uint8_t> message(input.begin(), input.end());
        message.push_back(0x80);

        while (message.size() % 64 != 56) {
            message.push_back(0x00);
        }

        uint64_t bitLen = input.size() * 8;
        for (int i = 7; i >= 0; --i) {
            message.push_back((bitLen >> (i * 8)) & 0xFF);
        }

        return message;
    }

    void processBlock(const uint8_t* block) {
        uint32_t W[64];

        for (int i = 0; i < 16; ++i) {
            W[i] = (block[i * 4] << 24) | (block[i * 4 + 1] << 16) |
                (block[i * 4 + 2] << 8) | block[i * 4 + 3];
        }

        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotateRight(W[i - 15], 7) ^ rotateRight(W[i - 15], 18) ^ (W[i - 15] >> 3);
            uint32_t s1 = rotateRight(W[i - 2], 17) ^ rotateRight(W[i - 2], 19) ^ (W[i - 2] >> 10);
            W[i] = W[i - 16] + s0 + W[i - 7] + s1;
        }

        uint32_t a = m_h[0], b = m_h[1], c = m_h[2], d = m_h[3],
            e = m_h[4], f = m_h[5], g = m_h[6], h = m_h[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t S1 = rotateRight(e, 6) ^ rotateRight(e, 11) ^ rotateRight(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t temp1 = h + S1 + ch + m_k[i] + W[i];
            uint32_t S0 = rotateRight(a, 2) ^ rotateRight(a, 13) ^ rotateRight(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        m_h[0] += a;
        m_h[1] += b;
        m_h[2] += c;
        m_h[3] += d;
        m_h[4] += e;
        m_h[5] += f;
        m_h[6] += g;
        m_h[7] += h;
    }
};