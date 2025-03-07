#pragma once

#pragma once

#include <string>
#include <sstream>
#include <ctime>

#include <QuasarEngine/Blockchain/BlockEvent.h>

class ItemBlockEvent : public BlockEvent {
private:
    std::string itemID;
    std::string action;
    int value;
    std::string timestamp;

public:
    ItemBlockEvent(const std::string& id, const std::string& act, int val)
        : itemID(id), action(act), value(val) {
        time_t now = time(0);
        timestamp = std::to_string(now);
    }

    std::string toString() const override {
        std::stringstream ss;
        ss << itemID << action << value << timestamp;
        return ss.str();
    }

    std::string printInfos() const override {
        std::stringstream ss;
        ss << "  -> Item: " << itemID << " | Action: " << action << " | Value: " << value << " | Time: " << timestamp << "\n";
        return ss.str();
    }
};