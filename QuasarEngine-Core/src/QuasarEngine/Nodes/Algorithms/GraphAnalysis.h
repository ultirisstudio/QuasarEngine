#pragma once

#include <QuasarEngine/Nodes/NodeGraph.h>

namespace QuasarEngine
{
    class GraphAnalysis
    {
    public:
        explicit GraphAnalysis(const NodeGraph* graph);

        std::vector<Node::NodeId> TopologicalSort() const;

    private:
        const NodeGraph* graph_;
    };
}
