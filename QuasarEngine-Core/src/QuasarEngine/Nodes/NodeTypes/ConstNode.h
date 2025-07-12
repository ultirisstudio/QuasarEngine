#pragma once

#include <QuasarEngine/Nodes/Node.h>

namespace QuasarEngine
{
    class ConstNode : public TypedNode
    {
    public:
        ConstNode(NodeId id, std::string typeName, PortType type, const std::any& value)
            : TypedNode(typeName, id), type_(type)
        {
            AddOutputPort("Value", type_);
            GetOutputPortValue("Value") = value;
        }

        virtual void Evaluate() override
        {
            
        }

    private:
        PortType type_;
    };
}