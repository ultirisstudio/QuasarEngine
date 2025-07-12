#pragma once

#include "JsonValue.h"

#include <unordered_map>
#include <memory>

namespace QuasarEngine
{
    class JsonObject : public JsonValue {
    public:
        JsonObject() = default;

        void add(const std::string& key, std::shared_ptr<JsonValue> value) {
            elements[key] = std::move(value);
        }

        std::string toString() const override {
            std::string result = "{";
            bool first = true;
            for (const auto& pair : elements) {
                if (!first) {
                    result += ", ";
                }
                first = false;
                result += "\"" + pair.first + "\": " + pair.second->toString();
            }
            result += "}";
            return result;
        }

    private:
        std::unordered_map<std::string, std::shared_ptr<JsonValue>> elements;
    };
}