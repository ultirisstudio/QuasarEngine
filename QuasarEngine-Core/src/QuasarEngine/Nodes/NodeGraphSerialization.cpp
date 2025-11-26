#include "qepch.h"
#include "NodeGraphSerialization.h"

#include <glm/glm.hpp>
#include <stdexcept>

namespace QuasarEngine
{
    using namespace QuasarEngine;

    static std::string GetTypeKeyForNode(const Node& node)
    {
        auto types = NodeRegistry::Instance().GetTypes();
        const std::string& display = node.GetTypeName();

        for (const auto& info : types)
        {
            if (info.displayName == display)
                return info.key;
        }

        return display;
    }

    static YAML::Node SerializePortValue(const std::any& value, PortType type)
    {
        YAML::Node n;
        try
        {
            switch (type)
            {
            case PortType::Float:  n = std::any_cast<float>(value);  break;
            case PortType::Int:    n = std::any_cast<int>(value);    break;
            case PortType::Bool:   n = std::any_cast<bool>(value);   break;
            case PortType::String: n = std::any_cast<std::string>(value); break;
            case PortType::Vec2:
            {
                auto v = std::any_cast<glm::vec2>(value);
                n.push_back(v.x); n.push_back(v.y);
                break;
            }
            case PortType::Vec3:
            {
                auto v = std::any_cast<glm::vec3>(value);
                n.push_back(v.x); n.push_back(v.y); n.push_back(v.z);
                break;
            }
            case PortType::Vec4:
            {
                auto v = std::any_cast<glm::vec4>(value);
                n.push_back(v.x); n.push_back(v.y); n.push_back(v.z); n.push_back(v.w);
                break;
            }
            default:
                break;
            }
        }
        catch (...)
        {
            
        }
        return n;
    }

    static void DeserializePortValue(const YAML::Node& n, std::any& value, PortType type)
    {
        if (!n) return;

        switch (type)
        {
        case PortType::Float:  value = n.as<float>();  break;
        case PortType::Int:    value = n.as<int>();    break;
        case PortType::Bool:   value = n.as<bool>();   break;
        case PortType::String: value = n.as<std::string>(); break;
        case PortType::Vec2:
        {
            glm::vec2 v{};
            if (n.IsSequence() && n.size() >= 2)
            {
                v.x = n[0].as<float>();
                v.y = n[1].as<float>();
            }
            value = v;
            break;
        }
        case PortType::Vec3:
        {
            glm::vec3 v{};
            if (n.IsSequence() && n.size() >= 3)
            {
                v.x = n[0].as<float>();
                v.y = n[1].as<float>();
                v.z = n[2].as<float>();
            }
            value = v;
            break;
        }
        case PortType::Vec4:
        {
            glm::vec4 v{};
            if (n.IsSequence() && n.size() >= 4)
            {
                v.x = n[0].as<float>();
                v.y = n[1].as<float>();
                v.z = n[2].as<float>();
                v.w = n[3].as<float>();
            }
            value = v;
            break;
        }
        default:
            break;
        }
    }

    YAML::Node SerializeNodeGraph(const NodeGraph& graph)
    {
        YAML::Node root;

        YAML::Node nodes = YAML::Node(YAML::NodeType::Sequence);
        for (const auto& [id, node] : graph.GetNodes())
        {
            YAML::Node n;
            n["id"] = id;
            n["type"] = GetTypeKeyForNode(*node);

            YAML::Node inPorts;
            for (const auto& p : node->GetInputPorts())
            {
                YAML::Node pn;
                pn["name"] = p.name;
                pn["type"] = static_cast<int>(p.type);
                pn["value"] = SerializePortValue(p.value, p.type);
                inPorts.push_back(pn);
            }
            n["inputs"] = inPorts;

            YAML::Node outPorts;
            for (const auto& p : node->GetOutputPorts())
            {
                YAML::Node pn;
                pn["name"] = p.name;
                pn["type"] = static_cast<int>(p.type);
                pn["value"] = SerializePortValue(p.value, p.type);
                outPorts.push_back(pn);
            }
            n["outputs"] = outPorts;

            YAML::Node props;
            node->SerializeProperties(props);
            n["properties"] = props;

            nodes.push_back(n);
        }
        root["nodes"] = nodes;

        YAML::Node conns = YAML::Node(YAML::NodeType::Sequence);
        for (const auto& c : graph.GetConnections())
        {
            auto from = c->fromNode.lock();
            auto to = c->toNode.lock();
            if (!from || !to) continue;

            YAML::Node cn;
            cn["fromNode"] = from->GetId();
            cn["fromPort"] = c->fromPort;
            cn["toNode"] = to->GetId();
            cn["toPort"] = c->toPort;
            cn["type"] = static_cast<int>(c->portType);
            conns.push_back(cn);
        }
        root["connections"] = conns;

        return root;
    }

    void DeserializeNodeGraph(const YAML::Node& root, NodeGraph& graph, NodeRegistry& registry)
    {
        graph = NodeGraph();

        const auto nodes = root["nodes"];
        if (!nodes || !nodes.IsSequence())
            return;

        std::unordered_map<Node::NodeId, std::shared_ptr<Node>> createdNodes;

        for (const auto& n : nodes)
        {
            Node::NodeId id = n["id"].as<Node::NodeId>();
            std::string type = n["type"].as<std::string>();

            auto node = registry.Create(type, id);
            if (!node)
                continue;

            if (auto inPorts = n["inputs"]; inPorts && inPorts.IsSequence())
            {
                auto& ports = node->GetInputPorts();
                ports.clear();
                for (const auto& pn : inPorts)
                {
                    std::string name = pn["name"].as<std::string>();
                    PortType pt = static_cast<PortType>(pn["type"].as<int>());
                    node->AddInputPort(name, pt);

                    auto& val = node->GetInputPortValue(name);
                    DeserializePortValue(pn["value"], val, pt);
                }
            }

            if (auto outPorts = n["outputs"]; outPorts && outPorts.IsSequence())
            {
                auto& ports = node->GetOutputPorts();
                ports.clear();
                for (const auto& pn : outPorts)
                {
                    std::string name = pn["name"].as<std::string>();
                    PortType pt = static_cast<PortType>(pn["type"].as<int>());
                    node->AddOutputPort(name, pt);

                    auto& val = node->GetOutputPortValue(name);
                    DeserializePortValue(pn["value"], val, pt);
                }
            }

            if (auto props = n["properties"])
                node->DeserializeProperties(props);

            graph.AddNode(node);
            createdNodes[id] = node;
        }

        const auto conns = root["connections"];
        if (conns && conns.IsSequence())
        {
            for (const auto& cn : conns)
            {
                Node::NodeId fromId = cn["fromNode"].as<Node::NodeId>();
                Node::NodeId toId = cn["toNode"].as<Node::NodeId>();
                std::string fromPort = cn["fromPort"].as<std::string>();
                std::string toPort = cn["toPort"].as<std::string>();

                graph.Connect(fromId, fromPort, toId, toPort);
            }
        }
    }
}