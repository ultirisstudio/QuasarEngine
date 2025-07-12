#pragma once

#include <QuasarEngine/Nodes/NodeGraph.h>
#include "QuasarEngine/Nodes/NodeTypes/ASTNode.h"

namespace QuasarEngine
{
    class ASTGenerator
    {
    public:
        explicit ASTGenerator(const NodeGraph* graph);
        ASTNode* Generate();
    private:
        const NodeGraph* graph_;
    };
}
