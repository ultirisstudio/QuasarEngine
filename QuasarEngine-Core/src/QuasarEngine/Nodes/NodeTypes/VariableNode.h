#pragma once

#include <QuasarEngine/Nodes/Node.h>

namespace QuasarEngine
{
    class VariableNode : public TypedNode
    {
    public:
        VariableNode(NodeId id, std::string typeName, PortType type = PortType::Float)
            : TypedNode(typeName, id), varType_(type)
        {
            AddOutputPort("Value", varType_);
        }

        void SetValue(const std::any& val)
        {
            GetOutputPortValue("Value") = val;
        }

        std::any GetValue() const
        {
            return outputPorts_[0].value;
        }

        virtual void Evaluate() override
        {
            
        }

    private:
        PortType varType_;
    };
}