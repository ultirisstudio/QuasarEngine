#pragma once

#include <QuasarEngine/Nodes/Node.h>
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

namespace QuasarEngine
{
    class ConstNode : public TypedNode
    {
    public:
        ConstNode(NodeId id, std::string typeName,
            PortType type, const std::any& value)
            : TypedNode(typeName, id)
            , m_Type(type)
            , m_DefaultValue(value)
        {
            switch (m_Type)
            {
            case PortType::Float:
            case PortType::Int:
            case PortType::Bool:
            case PortType::String:
                AddInputPort("In", m_Type);
                AddOutputPort("Value", m_Type);
                break;

            case PortType::Vec2:
                AddInputPort("X", PortType::Float);
                AddInputPort("Y", PortType::Float);
                AddOutputPort("Value", PortType::Vec2);
                break;

            case PortType::Vec3:
                AddInputPort("X", PortType::Float);
                AddInputPort("Y", PortType::Float);
                AddInputPort("Z", PortType::Float);
                AddOutputPort("Value", PortType::Vec3);
                break;

            case PortType::Vec4:
                AddInputPort("X", PortType::Float);
                AddInputPort("Y", PortType::Float);
                AddInputPort("Z", PortType::Float);
                AddInputPort("W", PortType::Float);
                AddOutputPort("Value", PortType::Vec4);
                break;

            default:
                break;
            }

            try
            {
                GetOutputPortValue("Value") = m_DefaultValue;
            }
            catch (...)
            {
                
            }
        }

        PortType GetConstType() const { return m_Type; }

        std::any GetDefaultValue() const { return m_DefaultValue; }

        void SetDefaultValue(const std::any& value)
        {
            m_DefaultValue = value;
            try
            {
                GetOutputPortValue("Value") = m_DefaultValue;
            }
            catch (...) {}
        }

        virtual void Evaluate() override
        {
            switch (m_Type)
            {
            case PortType::Float:
            {
                float def = 0.0f;
                try { def = std::any_cast<float>(m_DefaultValue); }
                catch (...) {}
                float v = def;

                if (Port* in = FindInputPort("In"))
                {
                    if (in->value.has_value())
                    {
                        try { v = std::any_cast<float>(in->value); }
                        catch (...) {}
                    }
                }

                SetOutput("Value", v);
                break;
            }
            case PortType::Int:
            {
                int def = 0;
                try { def = std::any_cast<int>(m_DefaultValue); }
                catch (...) {}
                int v = def;

                if (Port* in = FindInputPort("In"))
                {
                    if (in->value.has_value())
                    {
                        try { v = std::any_cast<int>(in->value); }
                        catch (...) {}
                    }
                }

                SetOutput("Value", v);
                break;
            }
            case PortType::Bool:
            {
                bool def = false;
                try { def = std::any_cast<bool>(m_DefaultValue); }
                catch (...) {}
                bool v = def;

                if (Port* in = FindInputPort("In"))
                {
                    if (in->value.has_value())
                    {
                        try { v = std::any_cast<bool>(in->value); }
                        catch (...) {}
                    }
                }

                SetOutput("Value", v);
                break;
            }
            case PortType::String:
            {
                std::string def;
                try { def = std::any_cast<std::string>(m_DefaultValue); }
                catch (...) {}
                std::string v = def;

                if (Port* in = FindInputPort("In"))
                {
                    if (in->value.has_value())
                    {
                        try { v = std::any_cast<std::string>(in->value); }
                        catch (...) {}
                    }
                }

                SetOutput("Value", v);
                break;
            }
            case PortType::Vec2:
            {
                glm::vec2 def(0.0f);
                try { def = std::any_cast<glm::vec2>(m_DefaultValue); }
                catch (...) {}
                glm::vec2 v = def;

                if (Port* inX = FindInputPort("X"))
                {
                    if (inX->value.has_value())
                    {
                        try { v.x = std::any_cast<float>(inX->value); }
                        catch (...) {}
                    }
                }
                if (Port* inY = FindInputPort("Y"))
                {
                    if (inY->value.has_value())
                    {
                        try { v.y = std::any_cast<float>(inY->value); }
                        catch (...) {}
                    }
                }

                SetOutput("Value", v);
                break;
            }
            case PortType::Vec3:
            {
                glm::vec3 def(0.0f);
                try { def = std::any_cast<glm::vec3>(m_DefaultValue); }
                catch (...) {}
                glm::vec3 v = def;

                if (Port* inX = FindInputPort("X"))
                {
                    if (inX->value.has_value())
                    {
                        try { v.x = std::any_cast<float>(inX->value); }
                        catch (...) {}
                    }
                }
                if (Port* inY = FindInputPort("Y"))
                {
                    if (inY->value.has_value())
                    {
                        try { v.y = std::any_cast<float>(inY->value); }
                        catch (...) {}
                    }
                }
                if (Port* inZ = FindInputPort("Z"))
                {
                    if (inZ->value.has_value())
                    {
                        try { v.z = std::any_cast<float>(inZ->value); }
                        catch (...) {}
                    }
                }

                SetOutput("Value", v);
                break;
            }
            case PortType::Vec4:
            {
                glm::vec4 def(0.0f);
                try { def = std::any_cast<glm::vec4>(m_DefaultValue); }
                catch (...) {}
                glm::vec4 v = def;

                if (Port* inX = FindInputPort("X"))
                {
                    if (inX->value.has_value())
                    {
                        try { v.x = std::any_cast<float>(inX->value); }
                        catch (...) {}
                    }
                }
                if (Port* inY = FindInputPort("Y"))
                {
                    if (inY->value.has_value())
                    {
                        try { v.y = std::any_cast<float>(inY->value); }
                        catch (...) {}
                    }
                }
                if (Port* inZ = FindInputPort("Z"))
                {
                    if (inZ->value.has_value())
                    {
                        try { v.z = std::any_cast<float>(inZ->value); }
                        catch (...) {}
                    }
                }
                if (Port* inW = FindInputPort("W"))
                {
                    if (inW->value.has_value())
                    {
                        try { v.w = std::any_cast<float>(inW->value); }
                        catch (...) {}
                    }
                }

                SetOutput("Value", v);
                break;
            }
            default:
                break;
            }
        }

        virtual void SerializeProperties(YAML::Node& out) const override
        {
            out["portType"] = static_cast<int>(m_Type);

            switch (m_Type)
            {
            case PortType::Float:
                out["default"] = m_DefaultValue.has_value()
                    ? std::any_cast<float>(m_DefaultValue)
                    : 0.0f;
                break;
            case PortType::Int:
                out["default"] = m_DefaultValue.has_value()
                    ? std::any_cast<int>(m_DefaultValue)
                    : 0;
                break;
            case PortType::Bool:
                out["default"] = m_DefaultValue.has_value()
                    ? std::any_cast<bool>(m_DefaultValue)
                    : false;
                break;
            case PortType::String:
                out["default"] = m_DefaultValue.has_value()
                    ? std::any_cast<std::string>(m_DefaultValue)
                    : std::string{};
                break;
            case PortType::Vec2:
            {
                glm::vec2 v(0.0f);
                try { v = std::any_cast<glm::vec2>(m_DefaultValue); }
                catch (...) {}
                YAML::Node arr;
                arr.push_back(v.x);
                arr.push_back(v.y);
                out["default"] = arr;
                break;
            }
            case PortType::Vec3:
            {
                glm::vec3 v(0.0f);
                try { v = std::any_cast<glm::vec3>(m_DefaultValue); }
                catch (...) {}
                YAML::Node arr;
                arr.push_back(v.x);
                arr.push_back(v.y);
                arr.push_back(v.z);
                out["default"] = arr;
                break;
            }
            case PortType::Vec4:
            {
                glm::vec4 v(0.0f);
                try { v = std::any_cast<glm::vec4>(m_DefaultValue); }
                catch (...) {}
                YAML::Node arr;
                arr.push_back(v.x);
                arr.push_back(v.y);
                arr.push_back(v.z);
                arr.push_back(v.w);
                out["default"] = arr;
                break;
            }
            default:
                break;
            }
        }

        virtual void DeserializeProperties(const YAML::Node& in) override
        {
            if (auto t = in["portType"])
                m_Type = static_cast<PortType>(t.as<int>());

            auto def = in["default"];

            if (!def)
            {
                try
                {
                    m_DefaultValue = GetOutputPortValue("Value");
                }
                catch (...) {}
                return;
            }

            switch (m_Type)
            {
            case PortType::Float:
                m_DefaultValue = def.as<float>();
                break;
            case PortType::Int:
                m_DefaultValue = def.as<int>();
                break;
            case PortType::Bool:
                m_DefaultValue = def.as<bool>();
                break;
            case PortType::String:
                m_DefaultValue = def.as<std::string>();
                break;
            case PortType::Vec2:
            {
                glm::vec2 v(0.0f);
                if (def.IsSequence() && def.size() >= 2)
                {
                    v.x = def[0].as<float>();
                    v.y = def[1].as<float>();
                }
                m_DefaultValue = v;
                break;
            }
            case PortType::Vec3:
            {
                glm::vec3 v(0.0f);
                if (def.IsSequence() && def.size() >= 3)
                {
                    v.x = def[0].as<float>();
                    v.y = def[1].as<float>();
                    v.z = def[2].as<float>();
                }
                m_DefaultValue = v;
                break;
            }
            case PortType::Vec4:
            {
                glm::vec4 v(0.0f);
                if (def.IsSequence() && def.size() >= 4)
                {
                    v.x = def[0].as<float>();
                    v.y = def[1].as<float>();
                    v.z = def[2].as<float>();
                    v.w = def[3].as<float>();
                }
                m_DefaultValue = v;
                break;
            }
            default:
                break;
            }

            try
            {
                GetOutputPortValue("Value") = m_DefaultValue;
            }
            catch (...) {}
        }

    private:
        PortType m_Type;
        std::any m_DefaultValue;
    };
}