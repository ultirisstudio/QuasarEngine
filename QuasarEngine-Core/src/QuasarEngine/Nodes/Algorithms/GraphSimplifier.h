#pragma once

#include "QuasarEngine/Nodes/NodeGraph.h"

namespace QuasarEngine
{
    class GraphSimplifier
    {
    public:
        explicit GraphSimplifier(NodeGraph* graph);
        void Simplify();

    private:
        NodeGraph* graph_;
    };
}
