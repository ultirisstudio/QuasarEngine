#pragma once

#include "JsonValue.h"

#include <vector>
#include <memory>

namespace QuasarEngine
{
    class JsonArray : public JsonValue {
    public:
        JsonArray() = default;

        void add(std::shared_ptr<JsonValue> value) {
            elements.push_back(std::move(value));
        }

        std::string toString() const override {
            std::string result = "[";
            for (size_t i = 0; i < elements.size(); ++i) {
                result += elements[i]->toString();
                if (i < elements.size() - 1) {
                    result += ", ";
                }
            }
            result += "]";
            return result;
        }

    private:
        std::vector<std::shared_ptr<JsonValue>> elements;
    };
}