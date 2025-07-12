#pragma once

#include "QuasarEngine/Nodes/NodeGraph.h"

namespace QuasarEngine
{
    class GraphEvaluator
    {
    public:
        explicit GraphEvaluator(NodeGraph* graph);

        std::unordered_map<Node::NodeId, std::any> Evaluate();

    private:
        NodeGraph* graph_;
        std::unordered_set<Node::NodeId> evaluated_;
        void EvaluateNode(Node* node);
    };
}
