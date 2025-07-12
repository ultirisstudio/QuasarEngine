#pragma once

#include "JsonObject.h"
#include "JsonArray.h"
#include "JsonPrimitive.h"

#include <string>
#include <memory>
#include <stdexcept>

namespace QuasarEngine
{
    class JsonParser {
    public:
        explicit JsonParser(const std::string& json) : jsonStr(json), pos(0) {}

        std::shared_ptr<JsonValue> parse() {
            skipWhitespace();
            return parseValue();
        }

    private:
        std::string jsonStr;
        size_t pos;

        void skipWhitespace() {
            while (pos < jsonStr.size() && isspace(jsonStr[pos])) {
                ++pos;
            }
        }

        std::shared_ptr<JsonValue> parseValue() {
            skipWhitespace();
            if (pos >= jsonStr.size()) {
                throw std::runtime_error("Unexpected end of JSON");
            }

            char c = jsonStr[pos];

            if (c == '{') return parseObject();
            if (c == '[') return parseArray();
            if (c == '"') return parseString();
            if (isdigit(c) || c == '-') return parseNumber();
            if (jsonStr.compare(pos, 4, "true") == 0) return parseTrue();
            if (jsonStr.compare(pos, 5, "false") == 0) return parseFalse();
            if (jsonStr.compare(pos, 4, "null") == 0) return parseNull();

            throw std::runtime_error("Invalid JSON value at position " + std::to_string(pos));
        }

        std::shared_ptr<JsonObject> parseObject() {
            auto obj = std::make_shared<JsonObject>();
            ++pos;
            skipWhitespace();

            while (pos < jsonStr.size() && jsonStr[pos] != '}') {
                auto key = parseString()->toString();
                //key = key.substr(0, key.size());
                skipWhitespace();

                if (jsonStr[pos] != ':') throw std::runtime_error("Expected ':' after key in object");
                ++pos;

                skipWhitespace();
                obj->add(key, parseValue());

                skipWhitespace();
                if (jsonStr[pos] == ',') ++pos;
                skipWhitespace();
            }

            if (jsonStr[pos] != '}') throw std::runtime_error("Expected '}' at end of object");
            ++pos;
            return obj;
        }

        std::shared_ptr<JsonArray> parseArray() {
            auto arr = std::make_shared<JsonArray>();
            ++pos;
            skipWhitespace();

            while (pos < jsonStr.size() && jsonStr[pos] != ']') {
                arr->add(parseValue());
                skipWhitespace();

                if (jsonStr[pos] == ',') ++pos;
                skipWhitespace();
            }

            if (jsonStr[pos] != ']') throw std::runtime_error("Expected ']' at end of array");
            ++pos;
            return arr;
        }

        std::shared_ptr<JsonPrimitive> parseString() {
            ++pos;
            std::string value;

            while (pos < jsonStr.size() && jsonStr[pos] != '"') {
                if (jsonStr[pos] == '\\') { // Handle escape characters
                    ++pos;
                    if (pos >= jsonStr.size()) throw std::runtime_error("Unexpected end of string");
                    if (jsonStr[pos] == '"') value += '"';
                    else if (jsonStr[pos] == '\\') value += '\\';
                    else if (jsonStr[pos] == 'n') value += '\n';
                    else if (jsonStr[pos] == 't') value += '\t';
                    else throw std::runtime_error("Invalid escape sequence");
                }
                else {
                    value += jsonStr[pos];
                }
                ++pos;
            }

            if (pos >= jsonStr.size() || jsonStr[pos] != '"') throw std::runtime_error("Unterminated string");
            ++pos;
            return std::make_shared<JsonPrimitive>(value);
        }

        std::shared_ptr<JsonPrimitive> parseNumber() {
            size_t start = pos;
            if (jsonStr[pos] == '-') ++pos;

            while (pos < jsonStr.size() && (isdigit(jsonStr[pos]) || jsonStr[pos] == '.')) {
                ++pos;
            }

            std::string numStr = jsonStr.substr(start, pos - start);
            if (numStr.find('.') != std::string::npos) {
                return std::make_shared<JsonPrimitive>(std::stof(numStr));
            }
            else {
                return std::make_shared<JsonPrimitive>(std::stoi(numStr));
            }
        }

        std::shared_ptr<JsonPrimitive> parseTrue() {
            pos += 4;
            return std::make_shared<JsonPrimitive>(true);
        }

        std::shared_ptr<JsonPrimitive> parseFalse() {
            pos += 5;
            return std::make_shared<JsonPrimitive>(false);
        }

        std::shared_ptr<JsonPrimitive> parseNull() {
            pos += 4;
            return nullptr;
        }
    };
}