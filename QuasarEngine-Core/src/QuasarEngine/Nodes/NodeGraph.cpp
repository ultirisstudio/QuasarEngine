#include "qepch.h"
#include "NodeGraph.h"

#include <stdexcept>
#include <algorithm>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace QuasarEngine
{
    Node::NodeId NodeGraph::m_NextId = 1;

    NodeGraph::NodePtr NodeGraph::CreateNode(const std::string& typeName)
    {
        NodeId id = GenerateId();
        auto node = std::make_shared<Node>(typeName, id);
        m_Nodes[id] = node;
        return node;
    }

    void NodeGraph::AddNode(NodePtr node)
    {
        if (!node)
            return;
        m_Nodes[node->GetId()] = node;
    }

    void NodeGraph::RemoveNode(NodeId id)
    {
        m_Nodes.erase(id);
        DisconnectAllForNode(id);
    }

    void NodeGraph::DisconnectAllForNode(NodeId id)
    {
        m_Connections.erase(
            std::remove_if(m_Connections.begin(), m_Connections.end(),
                [id](const auto& c)
                {
                    return (!c->fromNode.expired() && c->fromNode.lock()->GetId() == id)
                        || (!c->toNode.expired() && c->toNode.lock()->GetId() == id);
                }),
            m_Connections.end());
    }

    void NodeGraph::DisconnectAllInputs(NodeId toId, const std::string& toPort)
    {
        m_Connections.erase(
            std::remove_if(m_Connections.begin(), m_Connections.end(),
                [&](const auto& c)
                {
                    return !c->toNode.expired()
                        && c->toNode.lock()->GetId() == toId
                        && c->toPort == toPort;
                }),
            m_Connections.end());
    }

    NodeGraph::NodePtr NodeGraph::GetNode(NodeId id) const
    {
        auto it = m_Nodes.find(id);
        return it != m_Nodes.end() ? it->second : nullptr;
    }

    bool NodeGraph::Connect(NodeId fromId, const std::string& fromPort,
        NodeId toId, const std::string& toPort)
    {
        auto fromNode = GetNode(fromId);
        auto toNode = GetNode(toId);
        if (!fromNode || !toNode)
            return false;
        if (fromId == toId)
            return false;

        Port* fromP = fromNode->FindOutputPort(fromPort);
        Port* toP = toNode->FindInputPort(toPort);
        if (!fromP || !toP)
            return false;

        if (fromP->type != toP->type || fromP->type == PortType::Unknown)
            return false;

        for (const auto& c : m_Connections)
        {
            if (c->fromNode.lock() == fromNode &&
                c->toNode.lock() == toNode &&
                c->fromPort == fromPort &&
                c->toPort == toPort)
            {
                return false;
            }
        }

        DisconnectAllInputs(toId, toPort);

        auto conn = std::make_shared<NodeConnection>(fromNode, fromPort, toNode, toPort);
        conn->portType = fromP->type;

        for (int i = 0; i < (int)fromNode->GetOutputPorts().size(); ++i)
            if (fromNode->GetOutputPorts()[i].name == fromPort)
                conn->fromPortIndex = i;

        for (int i = 0; i < (int)toNode->GetInputPorts().size(); ++i)
            if (toNode->GetInputPorts()[i].name == toPort)
                conn->toPortIndex = i;

        m_Connections.push_back(conn);

        fromNode->OnConnectionsChanged();
        toNode->OnConnectionsChanged();

        return true;
    }

    void NodeGraph::Disconnect(NodeId fromId, const std::string& fromPort,
        NodeId toId, const std::string& toPort)
    {
        m_Connections.erase(
            std::remove_if(m_Connections.begin(), m_Connections.end(),
                [&](const auto& c)
                {
                    return !c->fromNode.expired()
                        && !c->toNode.expired()
                        && c->fromNode.lock()->GetId() == fromId
                        && c->fromPort == fromPort
                        && c->toNode.lock()->GetId() == toId
                        && c->toPort == toPort;
                }),
            m_Connections.end());
    }

    std::vector<std::shared_ptr<NodeConnection>>
        NodeGraph::GetConnectionsFrom(Node::NodeId id) const
    {
        std::vector<std::shared_ptr<NodeConnection>> out;
        for (const auto& conn : m_Connections)
        {
            if (!conn->fromNode.expired()
                && conn->fromNode.lock()->GetId() == id)
                out.push_back(conn);
        }
        return out;
    }

    std::vector<std::shared_ptr<NodeConnection>>
        NodeGraph::GetConnectionsTo(Node::NodeId id) const
    {
        std::vector<std::shared_ptr<NodeConnection>> result;
        for (const auto& conn : m_Connections)
        {
            if (!conn->toNode.expired()
                && conn->toNode.lock()->GetId() == id)
                result.push_back(conn);
        }
        return result;
    }

    std::shared_ptr<NodeConnection>
        NodeGraph::FindConnectionTo(Node::NodeId toId, const std::string& toPort) const
    {
        for (const auto& conn : m_Connections)
        {
            if (!conn->toNode.expired()
                && conn->toNode.lock()->GetId() == toId
                && conn->toPort == toPort)
                return conn;
        }
        return nullptr;
    }

    void NodeGraph::Evaluate()
    {
        if (m_Nodes.empty())
            return;

        std::unordered_map<NodeId, int> indegree;
        std::unordered_map<NodeId, std::vector<NodeId>> adj;

        for (const auto& [id, node] : m_Nodes)
        {
            indegree[id] = 0;
            adj[id];
        }

        for (const auto& conn : m_Connections)
        {
            auto from = conn->fromNode.lock();
            auto to = conn->toNode.lock();
            if (!from || !to)
                continue;

            NodeId fromId = from->GetId();
            NodeId toId = to->GetId();

            adj[fromId].push_back(toId);
            indegree[toId]++;
        }

        std::queue<NodeId> q;
        for (auto& [id, deg] : indegree)
            if (deg == 0) q.push(id);

        std::vector<NodeId> order;
        order.reserve(m_Nodes.size());

        while (!q.empty())
        {
            NodeId id = q.front(); q.pop();
            order.push_back(id);
            for (NodeId n : adj[id])
            {
                if (--indegree[n] == 0)
                    q.push(n);
            }
        }

        if (order.size() != m_Nodes.size())
        {
            std::unordered_set<NodeId> inOrder(order.begin(), order.end());
            for (const auto& [id, _] : m_Nodes)
                if (!inOrder.count(id))
                    order.push_back(id);
        }

        for (NodeId id : order)
        {
            auto node = GetNode(id);
            if (!node) continue;

            auto incoming = GetConnectionsTo(id);
            for (const auto& conn : incoming)
            {
                auto from = conn->fromNode.lock();
                auto to = conn->toNode.lock();
                if (!from || !to)
                    continue;

                try
                {
                    const std::any& srcVal = from->GetOutputPortValue(conn->fromPort);
                    to->GetInputPortValue(conn->toPort) = srcVal;
                }
                catch (const std::exception&)
                {
                    
                }
            }

            node->Evaluate();
        }
    }

    NodeGraph::NodeId NodeGraph::GenerateId()
    {
        return m_NextId++;
    }
}