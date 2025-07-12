#include "qepch.h"

#include "GraphAnalysis.h"
#include <unordered_set>
#include <vector>

namespace QuasarEngine
{
    GraphAnalysis::GraphAnalysis(const NodeGraph* graph) : graph_(graph) {}

    std::vector<Node::NodeId> GraphAnalysis::TopologicalSort() const
    {
        std::vector<Node::NodeId> result;
        std::unordered_set<Node::NodeId> visited;
        std::function<void(Node::NodeId)> visit = [&](Node::NodeId id)
            {
                if (visited.count(id)) return;
                visited.insert(id);
                for (const auto& conn : graph_->GetConnectionsFrom(id))
                    visit(conn->toNode.lock()->GetId());
                result.push_back(id);
            };
        for (const auto& pair : graph_->GetNodes())
            visit(pair.second->GetId());
        std::reverse(result.begin(), result.end());
        return result;
    }

}