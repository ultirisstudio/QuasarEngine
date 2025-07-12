#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include "Node.h"
#include "NodeConnection.h"

namespace QuasarEngine
{
    class NodeGraph
    {
    public:
        using NodePtr = std::shared_ptr<Node>;
        using NodeId = Node::NodeId;

        NodeGraph() = default;

        void AddNode(NodePtr node);

        NodePtr CreateNode(const std::string& typeName);
        void    RemoveNode(NodeId id);

        bool Connect(NodeId fromId, const std::string& fromPort,
            NodeId toId, const std::string& toPort);

        void Disconnect(NodeId fromId, const std::string& fromPort,
            NodeId toId, const std::string& toPort);

        NodePtr GetNode(NodeId id) const;
        const std::vector<std::shared_ptr<NodeConnection>>& GetConnections() const { return connections_; }
        const std::unordered_map<NodeId, NodePtr>& GetNodes() const { return nodes_; }

        std::vector<std::shared_ptr<NodeConnection>> GetConnectionsFrom(Node::NodeId id) const;
        std::vector<std::shared_ptr<NodeConnection>> GetConnectionsTo(Node::NodeId id) const;
        std::shared_ptr<NodeConnection> FindConnectionTo(Node::NodeId toId, const std::string& toPort) const;

        void Evaluate();

        static NodeId GenerateId();

    private:
        std::unordered_map<NodeId, NodePtr> nodes_;
        std::vector<std::shared_ptr<NodeConnection>> connections_;

        static NodeId nextId_;
    };
}
 