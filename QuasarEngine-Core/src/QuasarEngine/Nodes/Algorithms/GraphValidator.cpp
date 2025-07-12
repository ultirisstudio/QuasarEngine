#include "qepch.h"

#include "GraphValidator.h"

#include <unordered_set>
#include <stack>

namespace QuasarEngine
{
    GraphValidator::GraphValidator(const NodeGraph* graph) : graph_(graph) {}

    bool GraphValidator::HasCycle() const
    {
        std::unordered_set<Node::NodeId> visited, stack;
        std::function<bool(Node::NodeId)> dfs = [&](Node::NodeId nid) -> bool
            {
                if (stack.count(nid)) return true;
                if (visited.count(nid)) return false;
                visited.insert(nid); stack.insert(nid);
                for (const auto& conn : graph_->GetConnectionsFrom(nid))
                {
                    if (dfs(conn->toNode.lock()->GetId())) return true;
                }
                stack.erase(nid);
                return false;
            };

        for (const auto& pair : graph_->GetNodes())
        {
            if (dfs(pair.second->GetId())) return true;
        }
        return false;
    }

    bool GraphValidator::ValidateTypes() const
    {
        for (const auto& conn : graph_->GetConnections())
        {
            if (!conn->fromNode.expired() && !conn->toNode.expired())
            {
                auto fromNode = graph_->GetNode(conn->fromNode.lock()->GetId());
                auto toNode = graph_->GetNode(conn->toNode.lock()->GetId());
                if (!fromNode || !toNode) return false;
                auto fromType = fromNode->GetOutputPortType(conn->fromPort);
                auto toType = toNode->GetInputPortType(conn->toPort);
                if (fromType != toType) return false;
            }
        }
        return true;
    }

    std::vector<std::string> GraphValidator::GetErrors() const
    {
        std::vector<std::string> errors;
        if (HasCycle())
            errors.push_back("Le graphe contient un cycle !");
        if (!ValidateTypes())
            errors.push_back("Types incompatibles dans une connexion !");
        // Autres vérifs possibles...
        return errors;
    }
}