#pragma once

#include <string>
#include <sstream>
#include <ctime>

struct ItemEvent {
    std::string itemID;
    std::string action;
    int value;
    std::string timestamp;

    ItemEvent(const std::string& id, const std::string& act, int val)
        : itemID(id), action(act), value(val) {
        time_t now = time(0);
        timestamp = std::to_string(now);
    }

    std::string toString() const {
        std::stringstream ss;
        ss << itemID << action << value << timestamp;
        return ss.str();
    }
};