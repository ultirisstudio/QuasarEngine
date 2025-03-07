#pragma once

#include <string>
#include <vector>

#include <QuasarEngine/Blockchain/ItemEvent.h>
#include <QuasarEngine/Blockchain/Hasher.h>

class Block {
public:
    int index;
    std::string previousHash;
    std::vector<ItemEvent> events;
    std::string blockHash;
    std::string timestamp;

    Block(int idx, const std::string& prevHash, std::vector<ItemEvent> evts)
        : index(idx), previousHash(prevHash), events(evts) {
        time_t now = time(0);
        timestamp = std::to_string(now);
        blockHash = calculateHash();
    }

    std::string calculateHash() const {
        std::stringstream ss;
        ss << index << previousHash << timestamp;
        for (const auto& event : events) {
            ss << event.toString();
        }
        return Hasher::sha256(ss.str());
    }
};