#pragma once

#include <iostream>
#include <vector>
#include <iomanip>
#include <array>
#include <cstdint>
#include <cstring>

const uint8_t SBox[256] = {
    0x63, 0x7C, 0x77, 0x7B, 0xF0, 0x87, 0xF3, 0x73, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5,
    0xF1, 0x71, 0xD8, 0x31, 0x15, 0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2,
    0xEB, 0x27, 0xB2, 0x75, 0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29,
    0xE3, 0x2F, 0x84, 0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C,
    0x58, 0xCF, 0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F,
    0xA8, 0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xC1, 0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB, 0xE0,
    0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79, 0xE7, 0xC8,
    0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08, 0xBA, 0x78, 0x25,
    0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0x8B, 0x8A, 0x70, 0x3E, 0xB5, 0x66, 0x48,
    0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC9, 0x41, 0x1D, 0x6F, 0xA4, 0x61, 0x9E, 0xB7, 0xF2
};

const uint8_t invSBox[256] = {
        0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB, 0x7C,
        0x7B, 0xF2, 0x16, 0xCA, 0x8F, 0x4A, 0xA0, 0x8D, 0x3D, 0x66, 0x5B, 0x2C, 0x5E, 0xA1, 0x38, 0x1A, 0xDB,
        0x6B, 0x51, 0x7F, 0x52, 0x6F, 0x61, 0x78, 0xBD, 0xF9, 0xF6, 0xD4, 0x4B, 0x3E, 0xB6, 0x9C, 0xE5, 0xC6,
        0xE8, 0x48, 0xF7, 0xB4, 0xE9, 0xA4, 0x62, 0x7D, 0x51, 0xD2, 0x4D, 0x70, 0x68, 0x61, 0xBC, 0xFD, 0xB5,
        0x93, 0x1F, 0x0F, 0xDB, 0x3D, 0x53, 0xB7, 0x76, 0x95, 0x9C, 0x64, 0x35, 0x9F, 0x8E, 0x98, 0xB3, 0x43,
        0x0A, 0x0C, 0xE4, 0x93, 0xD9, 0x24, 0xEC, 0x91, 0xB1, 0x0D, 0x5F, 0x81, 0xF5, 0xD7, 0x67, 0x3C, 0x37
};

const uint32_t Rcon[10] = {
    0x01000000, 0x02000000, 0x04000000, 0x08000000,
    0x10000000, 0x20000000, 0x40000000, 0x80000000,
    0x1B000000, 0x36000000
};

class AES {
public:
    AES(const std::array<uint8_t, 16>& key) {
        keyExpansion(key);
    }

    std::array<uint8_t, 16> encrypt(const std::array<uint8_t, 16>& plaintext) {
        std::array<uint8_t, 16> state;
        std::copy(plaintext.begin(), plaintext.end(), state.begin());

        addRoundKey(state, 0);

        for (int round = 1; round < 10; ++round) {
            subBytes(state);
            shiftRows(state);
            mixColumns(state);
            addRoundKey(state, round);
        }

        subBytes(state);
        shiftRows(state);
        addRoundKey(state, 10);

        return state;
    }

    std::array<uint8_t, 16> decrypt(const std::array<uint8_t, 16>& ciphertext) {
        std::array<uint8_t, 16> state;
        std::copy(ciphertext.begin(), ciphertext.end(), state.begin());

        addRoundKey(state, 10);

        for (int round = 9; round > 0; --round) {
            invSubBytes(state);
            invShiftRows(state);
            addRoundKey(state, round);
            invMixColumns(state);
        }

        invSubBytes(state);
        invShiftRows(state);
        addRoundKey(state, 0);

        return state;
    }

private:
    uint32_t roundKeys[44];

    void keyExpansion(const std::array<uint8_t, 16>& key) {
        for (int i = 0; i < 4; ++i) {
            roundKeys[i] = (key[i * 4] << 24) | (key[i * 4 + 1] << 16) |
                (key[i * 4 + 2] << 8) | key[i * 4 + 3];
        }

        for (int i = 4; i < 44; ++i) {
            uint32_t temp = roundKeys[i - 1];
            if (i % 4 == 0) {
                temp = subWord(rotWord(temp)) ^ Rcon[i / 4 - 1];
            }
            roundKeys[i] = roundKeys[i - 4] ^ temp;
        }
    }

    uint32_t rotWord(uint32_t word) {
        return (word << 8) | (word >> 24);
    }

    uint32_t subWord(uint32_t word) {
        uint32_t result = 0;
        for (int i = 0; i < 4; ++i) {
            result |= (SBox[(word >> (8 * (3 - i))) & 0xFF] << (8 * (3 - i)));
        }
        return result;
    }

    void subBytes(std::array<uint8_t, 16>& state) {
        for (int i = 0; i < 16; ++i) {
            state[i] = SBox[state[i]];
        }
    }

    void shiftRows(std::array<uint8_t, 16>& state) {
        uint8_t temp;
        for (int i = 1; i < 4; ++i) {
            for (int j = 0; j < i; ++j) {
                temp = state[i * 4];
                state[i * 4] = state[i * 4 + 1];
                state[i * 4 + 1] = state[i * 4 + 2];
                state[i * 4 + 2] = state[i * 4 + 3];
                state[i * 4 + 3] = temp;
            }
        }
    }

    void mixColumns(std::array<uint8_t, 16>& state) {
        for (int i = 0; i < 4; ++i) {
            uint8_t a = state[i * 4];
            uint8_t b = state[i * 4 + 1];
            uint8_t c = state[i * 4 + 2];
            uint8_t d = state[i * 4 + 3];

            state[i * 4] = (a << 1) ^ (b << 1);
            state[i * 4 + 1] = (b << 1) ^ (c << 1);
            state[i * 4 + 2] = (c << 1) ^ (d << 1);
            state[i * 4 + 3] = (d << 1) ^ (a << 1);
        }
    }

    void addRoundKey(std::array<uint8_t, 16>& state, int round) {
        for (int i = 0; i < 4; ++i) {
            uint32_t roundKey = roundKeys[round * 4 + i];
            for (int j = 0; j < 4; ++j) {
                state[i * 4 + j] ^= (roundKey >> (8 * (3 - j))) & 0xFF;
            }
        }
    }

    void invSubBytes(std::array<uint8_t, 16>& state) {
        for (int i = 0; i < 16; ++i) {
            state[i] = invSBox[state[i]];
        }
    }

    void invShiftRows(std::array<uint8_t, 16>& state) {
        uint8_t temp;
        for (int i = 1; i < 4; ++i) {
            for (int j = 0; j < i; ++j) {
                temp = state[i * 4 + 3];
                state[i * 4 + 3] = state[i * 4 + 2];
                state[i * 4 + 2] = state[i * 4 + 1];
                state[i * 4 + 1] = state[i * 4];
                state[i * 4] = temp;
            }
        }

        /*uint8_t temp;
        temp = state[13]; state[13] = state[9]; state[9] = state[5]; state[5] = state[1]; state[1] = temp;
        temp = state[10]; state[10] = state[2]; state[2] = temp;
        temp = state[7]; state[7] = state[3]; state[3] = temp;*/
    }

    void invMixColumns(std::array<uint8_t, 16>& state) {
        for (int i = 0; i < 4; ++i) {
            uint8_t a = state[i * 4];
            uint8_t b = state[i * 4 + 1];
            uint8_t c = state[i * 4 + 2];
            uint8_t d = state[i * 4 + 3];

            state[i * 4] = (a << 1) ^ (b << 1);
            state[i * 4 + 1] = (b << 1) ^ (c << 1);
            state[i * 4 + 2] = (c << 1) ^ (d << 1);
            state[i * 4 + 3] = (d << 1) ^ (a << 1);
        }
    }
};