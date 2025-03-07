#pragma once

#include <vector>

#include <QuasarEngine/Blockchain/Blockchain.h>

class BlockchainManager {
private:
    std::vector<Blockchain> blockchains;

public:
    void addBlockchain() {
        blockchains.emplace_back();
    }

    Blockchain& getBlockchain(int index) {
        return blockchains[index];
    }
};