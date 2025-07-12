#pragma once

#include <string>
#include <vector>
#include <memory>
#include <any>

namespace QuasarEngine
{
    struct ASTNode
    {
        std::string op;
        std::vector<std::shared_ptr<ASTNode>> inputs;
        std::any value;
        std::string varName;
    };
}
