#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "QuasarEngine/Nodes/NodeTypes/VariableNode.h"
#include "QuasarEngine/Nodes/NodeTypes/MathNode.h"
#include "QuasarEngine/Nodes/NodeTypes/LogicNode.h"
#include "QuasarEngine/Nodes/NodeTypes/ConstNode.h"
#include "QuasarEngine/Nodes/Node.h"

namespace QuasarEngine
{
    class NodeRegistry
    {
    public:
        using NodeFactory = std::function<std::shared_ptr<Node>(Node::NodeId)>;

        static NodeRegistry& Instance()
        {
            static NodeRegistry instance;
            return instance;
        }

        void Register(const std::string& typeName, NodeFactory factory)
        {
            m_Factories[typeName] = factory;
        }

        std::shared_ptr<Node> Create(const std::string& typeName, Node::NodeId id) const
        {
            auto it = m_Factories.find(typeName);
            if (it != m_Factories.end())
                return it->second(id);
            return nullptr;
        }

        std::vector<std::string> GetRegisteredTypes() const
        {
            std::vector<std::string> names;
            for (auto& [name, _] : m_Factories) names.push_back(name);
            return names;
        }

    private:
        std::unordered_map<std::string, NodeFactory> m_Factories;
        NodeRegistry() = default;
    };
}
