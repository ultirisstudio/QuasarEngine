#pragma once

#include <string>
#include <vector>
#include <memory>
#include <any>
#include <cstdint>

namespace QuasarEngine
{
	class NodeGraph;
	struct NodeConnection;

    enum class PortType
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

    class Node : public std::enable_shared_from_this<Node>
    {
    public:
        struct Port
        {
            std::string name;
            PortType type;
            std::any value;

            Port(const std::string& n, PortType t)
                : name(n), type(t), value() {
            }
        };

        using NodeId = uint64_t;

        Node(const std::string& typeName, NodeId id);
        virtual ~Node() = default;

        NodeId GetId() const { return id_; }
        const std::string& GetTypeName() const { return typeName_; }

        const std::vector<Port>& GetInputPorts() const { return inputPorts_; }
        const std::vector<Port>& GetOutputPorts() const { return outputPorts_; }

        void AddInputPort(const std::string& name, PortType type);
        void AddOutputPort(const std::string& name, PortType type);

        std::any& GetInputPortValue(const std::string& name);
        std::any& GetOutputPortValue(const std::string& name);

        PortType GetOutputPortType(const std::string& name) const;
        PortType GetInputPortType(const std::string& name) const;

        bool HasOutputPort(const std::string& name);

        bool IsOutputNode() const;

        virtual void Evaluate() {}

    protected:
        NodeId id_;
        std::string typeName_;

        std::vector<Port> inputPorts_;
        std::vector<Port> outputPorts_;

        bool isOutput_;
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
