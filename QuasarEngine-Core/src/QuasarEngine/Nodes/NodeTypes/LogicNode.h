#pragma once

#include <QuasarEngine/Nodes/Node.h>
#include <yaml-cpp/yaml.h>
#include <stdexcept>

namespace QuasarEngine
{
    enum class LogicOp : uint8_t { And = 0, Or, Not, Xor, COUNT };

    class LogicNode : public TypedNode
    {
    public:
        LogicNode(NodeId id, std::string typeName, LogicOp op = LogicOp::And)
            : TypedNode(typeName, id)
            , m_Op(op)
        {
            AddInputPort("A", PortType::Bool);
            AddInputPort("B", PortType::Bool);
            AddOutputPort("Result", PortType::Bool);
        }

        LogicOp GetOperation() const { return m_Op; }
        void SetOperation(LogicOp op) { m_Op = op; }

        virtual void Evaluate() override
        {
            bool a = GetInput<bool>("A", false);
            bool b = (m_Op != LogicOp::Not) ? GetInput<bool>("B", false) : false;

            bool result = false;
            switch (m_Op)
            {
            case LogicOp::And: result = a && b; break;
            case LogicOp::Or:  result = a || b; break;
            case LogicOp::Not: result = !a;     break;
            case LogicOp::Xor: result = a != b; break;
            default: break;
            }

            SetOutput("Result", result);
        }

        virtual void SerializeProperties(YAML::Node& out) const override
        {
            out["logicOp"] = static_cast<int>(m_Op);
        }

        virtual void DeserializeProperties(const YAML::Node& in) override
        {
            if (auto m = in["logicOp"])
                m_Op = static_cast<LogicOp>(m.as<int>());
        }

    private:
        LogicOp m_Op;
    };
}