#pragma once

#include <QuasarEngine/Nodes/Node.h>
#include <yaml-cpp/yaml.h>

namespace QuasarEngine
{
    class VariableNode : public TypedNode
    {
    public:
        VariableNode(NodeId id, std::string typeName, PortType type = PortType::Float)
            : TypedNode(typeName, id)
            , m_VarType(type)
        {
            AddOutputPort("Value", m_VarType);
        }

        void SetValue(const std::any& val)
        {
            GetOutputPortValue("Value") = val;
        }

        std::any GetValue() const
        {
            return GetOutputPortValue("Value");
        }

        PortType GetVarType() const { return m_VarType; }

        virtual void Evaluate() override
        {
            
        }

        virtual void SerializeProperties(YAML::Node& out) const override
        {
            out["portType"] = static_cast<int>(m_VarType);
        }

        virtual void DeserializeProperties(const YAML::Node& in) override
        {
            if (auto t = in["portType"])
                m_VarType = static_cast<PortType>(t.as<int>());
        }

    private:
        PortType m_VarType;
    };
}