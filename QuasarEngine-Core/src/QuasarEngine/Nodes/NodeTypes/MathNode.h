#pragma once

#include <QuasarEngine/Nodes/Node.h>
#include <yaml-cpp/yaml.h>
#include <stdexcept>

namespace QuasarEngine
{
    enum class MathOp : uint8_t { Add = 0, Sub, Mul, Div, COUNT };

    class MathNode : public TypedNode
    {
    public:
        MathNode(NodeId id, std::string typeName, MathOp op = MathOp::Add)
            : TypedNode(typeName, id)
            , m_Op(op)
        {
            AddInputPort("A", PortType::Float);
            AddInputPort("B", PortType::Float);
            AddOutputPort("Result", PortType::Float);
        }

        MathOp GetOperation() const { return m_Op; }
        void SetOperation(MathOp op) { m_Op = op; }

        virtual void Evaluate() override
        {
            float a = GetInput<float>("A", 0.0f);
            float b = GetInput<float>("B", 0.0f);

            float result = 0.0f;
            switch (m_Op)
            {
            case MathOp::Add: result = a + b; break;
            case MathOp::Sub: result = a - b; break;
            case MathOp::Mul: result = a * b; break;
            case MathOp::Div: result = (b != 0.0f) ? a / b : 0.0f; break;
            default: break;
            }

            SetOutput("Result", result);
        }

        virtual void SerializeProperties(YAML::Node& out) const override
        {
            out["mathOp"] = static_cast<int>(m_Op);
        }

        virtual void DeserializeProperties(const YAML::Node& in) override
        {
            if (auto m = in["mathOp"])
                m_Op = static_cast<MathOp>(m.as<int>());
        }

    private:
        MathOp m_Op;
    };
}