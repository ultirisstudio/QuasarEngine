#include "qepch.h"

#include "Node.h"

namespace QuasarEngine
{
    Node::Node(const std::string& typeName, NodeId id)
        : id_(id), typeName_(typeName), isOutput_(false)
    {
    }

    void Node::AddInputPort(const std::string& name, PortType type)
    {
        inputPorts_.emplace_back(name, type);
    }

    void Node::AddOutputPort(const std::string& name, PortType type)
    {
        outputPorts_.emplace_back(name, type);
    }

    std::any& Node::GetInputPortValue(const std::string& name)
    {
        for (auto& port : inputPorts_)
            if (port.name == name)
                return port.value;
        throw std::runtime_error("Input port not found: " + name);
    }

    std::any& Node::GetOutputPortValue(const std::string& name)
    {
        for (auto& port : outputPorts_)
            if (port.name == name)
                return port.value;
        throw std::runtime_error("Output port not found: " + name);
    }

    PortType Node::GetOutputPortType(const std::string& name) const
    {
        for (const auto& port : outputPorts_)
            if (port.name == name)
                return port.type;
        return PortType::Unknown;
    }

    PortType Node::GetInputPortType(const std::string& name) const
    {
        for (const auto& port : inputPorts_)
            if (port.name == name)
                return port.type;
        return PortType::Unknown;
    }

    bool Node::HasOutputPort(const std::string& name)
    {
        for (const auto& port : outputPorts_)
            if (port.name == name)
                return true;
        return false;
    }

    bool Node::IsOutputNode() const
    {
        return isOutput_;
    }
}
