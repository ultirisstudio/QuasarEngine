#pragma once

#include <iostream>
#include <vector>
#include <string>

#include <QuasarEngine/Blockchain/Block.h>

class Blockchain {
private:
    std::vector<Block> chain;

    Block createGenesisBlock() {
        return Block(0, "0", {});
    }

public:
    Blockchain() {
        chain.push_back(createGenesisBlock());
    }

    void addBlock(const std::vector<BlockEvent>& events) {
        int index = chain.size();
        std::string prevHash = chain.back().blockHash;
        Block newBlock(index, prevHash, events);
        chain.push_back(newBlock);
    }

    void printBlockchain() const {
        for (const auto& block : chain) {
            std::cout << "Block #" << block.index << " | Hash: " << block.blockHash << "\n";
            for (const auto& event : block.events) {
                std::cout << event.printInfos() << std::endl;
            }
            std::cout << "------------------------\n";
        }
    }
};