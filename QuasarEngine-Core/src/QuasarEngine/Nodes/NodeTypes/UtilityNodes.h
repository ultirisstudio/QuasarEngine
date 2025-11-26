#pragma once

#include <QuasarEngine/Nodes/Node.h>
#include <yaml-cpp/yaml.h>
#include <random>
#include <ctime>

namespace QuasarEngine
{
    class TimeNode : public TypedNode
    {
    public:
        TimeNode(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddOutputPort("Time", PortType::Float);
        }

        void Evaluate() override
        {
            static float s_Time = 0.0f;
            s_Time += 0.016f;
            SetOutput("Time", s_Time);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    class RandomFloatNode : public TypedNode
    {
    public:
        RandomFloatNode(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("Min", PortType::Float);
            AddInputPort("Max", PortType::Float);
            AddOutputPort("Value", PortType::Float);
        }

        void Evaluate() override
        {
            float minV = 0.0f;
            float maxV = 1.0f;

            try { minV = std::any_cast<float>(GetInputPortValue("Min")); }
            catch (...) {}
            try { maxV = std::any_cast<float>(GetInputPortValue("Max")); }
            catch (...) {}

            if (minV > maxV)
                std::swap(minV, maxV);

            static std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));
            std::uniform_real_distribution<float> dist(minV, maxV);

            float v = dist(rng);
            SetOutput("Value", v);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    enum class CompareOp : uint8_t
    {
        Less = 0,
        LessEqual,
        Greater,
        GreaterEqual,
        Equal,
        NotEqual,
        COUNT
    };

    class CompareFloatNode : public TypedNode
    {
    public:
        CompareFloatNode(NodeId id, const std::string& typeName, CompareOp op = CompareOp::Less)
            : TypedNode(typeName, id)
            , m_Op(op)
        {
            AddInputPort("A", PortType::Float);
            AddInputPort("B", PortType::Float);
            AddOutputPort("Result", PortType::Bool);
        }

        void SetOperation(CompareOp op) { m_Op = op; }
        CompareOp GetOperation() const { return m_Op; }

        void Evaluate() override
        {
            float a = 0.0f;
            float b = 0.0f;

            try { a = std::any_cast<float>(GetInputPortValue("A")); }
            catch (...) {}
            try { b = std::any_cast<float>(GetInputPortValue("B")); }
            catch (...) {}

            bool result = false;
            switch (m_Op)
            {
            case CompareOp::Less:         result = (a < b); break;
            case CompareOp::LessEqual:    result = (a <= b); break;
            case CompareOp::Greater:      result = (a > b); break;
            case CompareOp::GreaterEqual: result = (a >= b); break;
            case CompareOp::Equal:        result = (a == b); break;
            case CompareOp::NotEqual:     result = (a != b); break;
            default: break;
            }

            SetOutput("Result", result);
        }

        void SerializeProperties(YAML::Node& out) const override
        {
            out["compareOp"] = static_cast<int>(m_Op);
        }

        void DeserializeProperties(const YAML::Node& in) override
        {
            if (auto m = in["compareOp"])
                m_Op = static_cast<CompareOp>(m.as<int>());
        }

    private:
        CompareOp m_Op;
    };
}
