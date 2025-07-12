#pragma once

#include <QuasarEngine/Nodes/Node.h>

#include <stdexcept>

namespace QuasarEngine
{
    enum class LogicOp : uint8_t { And = 0, Or, Not, Xor, COUNT };

    class LogicNode : public TypedNode
    {
    public:
        LogicNode(NodeId id, std::string typeName, LogicOp op = LogicOp::And)
            : TypedNode(typeName, id), op_(op)
        {
            if (op == LogicOp::Not)
            {
                AddInputPort("A", PortType::Bool);
            }
            else
            {
                AddInputPort("A", PortType::Bool);
                AddInputPort("B", PortType::Bool);
            }
            AddOutputPort("Result", PortType::Bool);
        }

        void SetOperation(LogicOp op)
        {
            op_ = op;
        }

        virtual void Evaluate() override
        {
            bool result = false;
            bool a = std::any_cast<bool>(GetInputPortValue("A"));
            bool b = (op_ != LogicOp::Not) ? std::any_cast<bool>(GetInputPortValue("B")) : false;
            switch (op_)
            {
            case LogicOp::And: result = a && b; break;
            case LogicOp::Or:  result = a || b; break;
            case LogicOp::Not: result = !a;     break;
            case LogicOp::Xor: result = a != b; break;
            }
            GetOutputPortValue("Result") = result;
        }

    private:
        LogicOp op_;
    };
}