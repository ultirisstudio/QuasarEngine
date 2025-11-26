#include "qepch.h"
#include "Node.h"

#include <stdexcept>

namespace QuasarEngine
{
    Node::Node(const std::string& typeName, NodeId id)
        : m_Id(id)
        , m_TypeName(typeName)
        , m_IsOutput(false)
    {
    }

    void Node::AddInputPort(const std::string& name, PortType type)
    {
        m_InputPorts.emplace_back(name, type);
    }

    void Node::AddOutputPort(const std::string& name, PortType type)
    {
        m_OutputPorts.emplace_back(name, type);
    }

    bool Node::HasInputPort(const std::string& name) const
    {
        for (const auto& p : m_InputPorts)
            if (p.name == name) return true;
        return false;
    }

    bool Node::HasOutputPort(const std::string& name) const
    {
        for (const auto& p : m_OutputPorts)
            if (p.name == name) return true;
        return false;
    }

    Port* Node::FindInputPort(const std::string& name)
    {
        for (auto& p : m_InputPorts)
            if (p.name == name) return &p;
        return nullptr;
    }

    Port* Node::FindOutputPort(const std::string& name)
    {
        for (auto& p : m_OutputPorts)
            if (p.name == name) return &p;
        return nullptr;
    }

    const Port* Node::FindInputPort(const std::string& name) const
    {
        for (const auto& p : m_InputPorts)
            if (p.name == name) return &p;
        return nullptr;
    }

    const Port* Node::FindOutputPort(const std::string& name) const
    {
        for (const auto& p : m_OutputPorts)
            if (p.name == name) return &p;
        return nullptr;
    }

    std::any& Node::GetInputPortValue(const std::string& name)
    {
        Port* p = FindInputPort(name);
        if (!p)
            throw std::runtime_error("Input port not found: " + name);
        return p->value;
    }

    std::any& Node::GetOutputPortValue(const std::string& name)
    {
        Port* p = FindOutputPort(name);
        if (!p)
            throw std::runtime_error("Output port not found: " + name);
        return p->value;
    }

    const std::any& Node::GetInputPortValue(const std::string& name) const
    {
        const Port* p = FindInputPort(name);
        if (!p)
            throw std::runtime_error("Input port not found: " + name);
        return p->value;
    }

    const std::any& Node::GetOutputPortValue(const std::string& name) const
    {
        const Port* p = FindOutputPort(name);
        if (!p)
            throw std::runtime_error("Output port not found: " + name);
        return p->value;
    }
}