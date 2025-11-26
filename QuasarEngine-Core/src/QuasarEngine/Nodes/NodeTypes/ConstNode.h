#pragma once

#include <QuasarEngine/Nodes/Node.h>
#include <yaml-cpp/yaml.h>

namespace QuasarEngine
{
    class ConstNode : public TypedNode
    {
    public:
        ConstNode(NodeId id, std::string typeName,
            PortType type, const std::any& value)
            : TypedNode(typeName, id)
            , m_Type(type)
        {
            AddOutputPort("Value", m_Type);
            GetOutputPortValue("Value") = value;
        }

        PortType GetConstType() const { return m_Type; }

        std::any GetValue() const { return GetOutputPortValue("Value"); }
        void SetValue(const std::any& v) { GetOutputPortValue("Value") = v; }

        virtual void Evaluate() override
        {
            
        }

        virtual void SerializeProperties(YAML::Node& out) const override
        {
            out["portType"] = static_cast<int>(m_Type);
        }

        virtual void DeserializeProperties(const YAML::Node& in) override
        {
            if (auto t = in["portType"])
                m_Type = static_cast<PortType>(t.as<int>());
        }

    private:
        PortType m_Type;
    };
}