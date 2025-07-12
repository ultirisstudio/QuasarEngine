#pragma once

#include "JsonValue.h"

#include <variant>
#include <sstream>

namespace QuasarEngine
{
    class JsonPrimitive : public JsonValue {
    public:
        using ValueType = std::variant<bool, int, float, std::string>;

        explicit JsonPrimitive(bool value) : data(value) {}
        explicit JsonPrimitive(int value) : data(value) {}
        explicit JsonPrimitive(float value) : data(value) {}
        explicit JsonPrimitive(const std::string& value) : data(value) {}

        std::string toString() const override {
            return std::visit([](auto&& arg) -> std::string {
                if constexpr (std::is_same_v<decltype(arg), std::string>)
                    return "\"" + arg + "\"";
                else {
                    std::ostringstream oss;
                    oss << arg;
                    return oss.str();
                }
                }, data);
        }

    private:
        ValueType data;
    };
}