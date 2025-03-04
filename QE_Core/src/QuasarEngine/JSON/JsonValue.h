#pragma once

#include <memory>
#include <string>

class JsonValue {
public:
    virtual ~JsonValue() = default;

    virtual std::string toString() const = 0;
};