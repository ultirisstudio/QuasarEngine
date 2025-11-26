#pragma once

#include <string>
#include <vector>
#include <memory>
#include <any>
#include <cstdint>

namespace YAML { class Node; }

namespace QuasarEngine
{
    class NodeGraph;
    struct NodeConnection;

    enum class PortType : uint8_t
    {
        Float = 0,
        Int,
        Bool,
        Vec2,
        Vec3,
        Vec4,
        String,
        Unknown,
        COUNT
    };

    struct Port
    {
        std::string name;
        PortType type;
        std::any value;

        Port() : type(PortType::Unknown) {}
        Port(std::string n, PortType t)
            : name(std::move(n)), type(t) {
        }
    };

    class Node : public std::enable_shared_from_this<Node>
    {
    public:
        using NodeId = std::uint32_t;

        Node(const std::string& typeName, NodeId id);
        virtual ~Node() = default;

        NodeId GetId() const noexcept { return m_Id; }
        const std::string& GetTypeName() const noexcept { return m_TypeName; }

        void AddInputPort(const std::string& name, PortType type);
        void AddOutputPort(const std::string& name, PortType type);

        const std::vector<Port>& GetInputPorts() const { return m_InputPorts; }
        const std::vector<Port>& GetOutputPorts() const { return m_OutputPorts; }
        std::vector<Port>& GetInputPorts() { return m_InputPorts; }
        std::vector<Port>& GetOutputPorts() { return m_OutputPorts; }

        bool HasInputPort(const std::string& name) const;
        bool HasOutputPort(const std::string& name) const;

        Port* FindInputPort(const std::string& name);
        Port* FindOutputPort(const std::string& name);
        const Port* FindInputPort(const std::string& name) const;
        const Port* FindOutputPort(const std::string& name) const;

        std::any& GetInputPortValue(const std::string& name);
        std::any& GetOutputPortValue(const std::string& name);
        const std::any& GetInputPortValue(const std::string& name) const;
        const std::any& GetOutputPortValue(const std::string& name) const;

        template<typename T>
        T GetInput(const std::string& name, const T& defaultValue = {}) const
        {
            const Port* p = FindInputPort(name);
            if (!p || !p->value.has_value())
                return defaultValue;

            try
            {
                return std::any_cast<T>(p->value);
            }
            catch (const std::bad_any_cast&)
            {
                return defaultValue;
            }
        }

        template<typename T>
        void SetOutput(const std::string& name, const T& value)
        {
            Port* p = FindOutputPort(name);
            if (!p)
                return;
            p->value = value;
        }

        virtual void Evaluate() {}

        bool IsOutputNode() const { return m_IsOutput; }
        void SetIsOutputNode(bool v) { m_IsOutput = v; }

        virtual void OnConnectionsChanged() {}

        virtual void SerializeProperties(YAML::Node& out) const {}
        virtual void DeserializeProperties(const YAML::Node& in) {}

    protected:
        NodeId m_Id;
        std::string m_TypeName;
        std::vector<Port> m_InputPorts;
        std::vector<Port> m_OutputPorts;
        bool m_IsOutput = false;
    };

    class TypedNode : public Node
    {
    public:
        TypedNode(const std::string& typeName, NodeId id)
            : Node(typeName, id) {
        }

        virtual void Evaluate() override = 0;
    };
}