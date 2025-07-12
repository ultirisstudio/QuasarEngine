#include "qepch.h"

#include "NodeGraph.h"
#include <stdexcept>
#include <algorithm>

namespace QuasarEngine
{
    Node::NodeId NodeGraph::nextId_ = 1;

    void NodeGraph::AddNode(NodePtr node)
    {
        nodes_[node->GetId()] = node;
    }

    NodeGraph::NodePtr NodeGraph::CreateNode(const std::string& typeName)
    {
        NodeId id = GenerateId();
        auto node = std::make_shared<Node>(typeName, id);
        nodes_[id] = node;
        return node;
    }

    void NodeGraph::RemoveNode(NodeId id)
    {
        nodes_.erase(id);

        connections_.erase(std::remove_if(connections_.begin(), connections_.end(),
            [id](const auto& c) {
                return (!c->fromNode.expired() && c->fromNode.lock()->GetId() == id)
                    || (!c->toNode.expired() && c->toNode.lock()->GetId() == id);
            }), connections_.end());
    }

    bool NodeGraph::Connect(NodeId fromId, const std::string& fromPort,
        NodeId toId, const std::string& toPort)
    {
        auto fromNode = GetNode(fromId);
        auto toNode = GetNode(toId);
        if (!fromNode || !toNode)
            return false;

        connections_.push_back(std::make_shared<NodeConnection>(fromNode, fromPort, toNode, toPort));
        return true;
    }

    void NodeGraph::Disconnect(NodeId fromId, const std::string& fromPort,
        NodeId toId, const std::string& toPort)
    {
        connections_.erase(std::remove_if(connections_.begin(), connections_.end(),
            [&](const auto& c) {
                return (!c->fromNode.expired() && !c->toNode.expired()
                    && c->fromNode.lock()->GetId() == fromId && c->fromPort == fromPort
                    && c->toNode.lock()->GetId() == toId && c->toPort == toPort);
            }), connections_.end());
    }

    NodeGraph::NodePtr NodeGraph::GetNode(NodeId id) const
    {
        auto it = nodes_.find(id);
        return it != nodes_.end() ? it->second : nullptr;
    }

    void NodeGraph::Evaluate()
    {
        for (const auto& [id, node] : nodes_)
            node->Evaluate();
    }

    std::vector<std::shared_ptr<NodeConnection>> NodeGraph::GetConnectionsFrom(Node::NodeId id) const
    {
        std::vector<std::shared_ptr<NodeConnection>> out;
        for (const auto& conn : connections_) {
            if (!conn->fromNode.expired())
	            if (conn->fromNode.lock()->GetId() == id)
	                out.push_back(conn);
        }
        return out;
    }

    std::vector<std::shared_ptr<NodeConnection>> NodeGraph::GetConnectionsTo(Node::NodeId id) const
    {
        std::vector<std::shared_ptr<NodeConnection>> result;
        for (const auto& conn : connections_)
        {
            if (!conn->toNode.expired())
	            if (conn->toNode.lock()->GetId() == id)
	                result.push_back(conn);
        }
        return result;
    }


    std::shared_ptr<NodeConnection> NodeGraph::FindConnectionTo(Node::NodeId toId, const std::string& toPort) const
    {
        for (const auto& conn : connections_) {
            if (!conn->toNode.expired())
	            if (conn->toNode.lock()->GetId() == toId && conn->toPort == toPort)
	                return conn;
        }
        return nullptr;
    }


    NodeGraph::NodeId NodeGraph::GenerateId()
    {
        return nextId_++;
    }
}