#pragma once

#include <stdexcept>
#include <sstream>

#include "Token.h"

namespace QuasarEngine
{
    struct QError : std::runtime_error {
        SourceLocation loc{};
        QError(SourceLocation l, std::string msg)
            : std::runtime_error([&] { std::ostringstream o; o << "L" << l.line << ":" << l.col << " " << msg; return o.str(); }()),
            loc(l) {
        }
        using std::runtime_error::runtime_error;
    };
}