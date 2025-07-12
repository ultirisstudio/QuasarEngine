#include "qepch.h"

#include "GraphEvaluator.h"

namespace QuasarEngine
{
    GraphEvaluator::GraphEvaluator(NodeGraph* graph) : graph_(graph) {}

    std::unordered_map<Node::NodeId, std::any> GraphEvaluator::Evaluate()
    {
        evaluated_.clear();
        for (const auto& node : graph_->GetNodes())
        {
            EvaluateNode(node.second.get());
        }

        std::unordered_map<Node::NodeId, std::any> results;
        for (const auto& node : graph_->GetNodes())
        {
            if (graph_->GetConnectionsFrom(node.second->GetId()).empty())
            {
                try {
                    if (node.second->HasOutputPort("Result"))
                        results[node.second->GetId()] = node.second->GetOutputPortValue("Result");
                    else if (node.second->HasOutputPort("Value"))
                        results[node.second->GetId()] = node.second->GetOutputPortValue("Value");
                }
                catch (...) {}
            }
        }
        return results;
    }

    void GraphEvaluator::EvaluateNode(Node* node)
    {
        if (!node || evaluated_.count(node->GetId()))
            return;

        for (const auto& input : node->GetInputPorts())
        {
            auto conn = graph_->FindConnectionTo(node->GetId(), input.name);
            if (conn)
            {
                if (!conn->fromNode.expired())
                {
                    Node* prev = graph_->GetNode(conn->fromNode.lock()->GetId()).get();
                    EvaluateNode(prev);
                    node->GetInputPortValue(input.name) = prev->GetOutputPortValue(conn->fromPort);
                }
            }
        }

        node->Evaluate();
        evaluated_.insert(node->GetId());
    }
}