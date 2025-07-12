#pragma once

#include <QuasarEngine/Nodes/Node.h>

#include <stdexcept>

namespace QuasarEngine
{
    enum class MathOp : uint8_t { Add = 0, Sub, Mul, Div, COUNT };

    class MathNode : public TypedNode
    {
    public:
        MathNode(NodeId id, std::string typeName, MathOp op = MathOp::Add)
            : TypedNode(typeName, id), op_(op)
        {
            AddInputPort("A", PortType::Float);
            AddInputPort("B", PortType::Float);
            AddOutputPort("Result", PortType::Float);
        }

        void SetOperation(MathOp op) { op_ = op; }
        MathOp GetOperation() const { return op_; }

        virtual void Evaluate() override
        {
            float a = std::any_cast<float>(GetInputPortValue("A"));
            float b = std::any_cast<float>(GetInputPortValue("B"));
            float result = 0.0f;
            switch (op_)
            {
            case MathOp::Add:   result = a + b; break;
            case MathOp::Sub:   result = a - b; break;
            case MathOp::Mul:   result = a * b; break;
            case MathOp::Div:   result = (b != 0.0f) ? a / b : 0.0f; break;
            }
            GetOutputPortValue("Result") = result;
        }

    private:
        MathOp op_;
    };
}