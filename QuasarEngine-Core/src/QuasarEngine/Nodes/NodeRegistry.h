#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "QuasarEngine/Nodes/Node.h"
#include "QuasarEngine/Nodes/NodeTypes/ConstNode.h"
#include "QuasarEngine/Nodes/NodeTypes/MathNode.h"
#include "QuasarEngine/Nodes/NodeTypes/LogicNode.h"
#include <QuasarEngine/Nodes/NodeTypes/VectorComponentsNode.h>
#include <QuasarEngine/Nodes/NodeTypes/TextureNode.h>
#include <QuasarEngine/Nodes/NodeTypes/MathUtilityNodes.h>
#include <QuasarEngine/Nodes/NodeTypes/UtilityNodes.h>
#include <QuasarEngine/Nodes/NodeTypes/TextureUtilityNodes.h>

namespace QuasarEngine
{
    class NodeRegistry
    {
    public:
        using NodeFactory = std::function<std::shared_ptr<Node>(Node::NodeId)>;

        struct NodeTypeInfo
        {
            std::string key;
            std::string displayName;
            std::string category;
            NodeFactory factory;

            bool operator<(const NodeTypeInfo& other) const noexcept
            {
                if (category == other.category)
                    return displayName < other.displayName;
                return category < other.category;
            }
        };

        static NodeRegistry& Instance()
        {
            static NodeRegistry s_Instance;
            return s_Instance;
        }

        void Register(const std::string& key,
            const std::string& displayName,
            const std::string& category,
            NodeFactory factory)
        {
            NodeTypeInfo info;
            info.key = key;
            info.displayName = displayName;
            info.category = category;
            info.factory = std::move(factory);
            m_Types[key] = std::move(info);
        }

        std::shared_ptr<Node> Create(const std::string& key, Node::NodeId id) const
        {
            auto it = m_Types.find(key);
            if (it == m_Types.end())
                return nullptr;
            return it->second.factory(id);
        }

        std::vector<NodeTypeInfo> GetTypes() const
        {
            std::vector<NodeTypeInfo> out;
            out.reserve(m_Types.size());
            for (const auto& [_, info] : m_Types)
                out.push_back(info);
            return out;
        }

        std::vector<std::string> GetRegisteredKeys() const
        {
            std::vector<std::string> names;
            names.reserve(m_Types.size());
            for (auto& [name, _] : m_Types)
                names.push_back(name);
            return names;
        }

    private:
        std::unordered_map<std::string, NodeTypeInfo> m_Types;
        NodeRegistry() = default;
    };
}
