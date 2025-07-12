#include "qepch.h"

#include "GraphSimplifier.h"

namespace QuasarEngine
{
    GraphSimplifier::GraphSimplifier(NodeGraph* graph) : graph_(graph) {}

    void GraphSimplifier::Simplify()
    {
        std::vector<Node::NodeId> toDelete;
        for (const auto& node : graph_->GetNodes())
        {
            if (graph_->GetConnectionsFrom(node.second->GetId()).empty() &&
                graph_->GetConnectionsTo(node.second->GetId()).empty())
            {
                toDelete.push_back(node.second->GetId());
            }
        }
        for (Node::NodeId id : toDelete)
            graph_->RemoveNode(id);
    }
}