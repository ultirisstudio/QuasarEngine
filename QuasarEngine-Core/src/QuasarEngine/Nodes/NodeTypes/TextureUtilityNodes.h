#pragma once

#include <QuasarEngine/Nodes/Node.h>
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

namespace QuasarEngine
{
    class TextureCoordinateNode : public TypedNode
    {
    public:
        TextureCoordinateNode(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddOutputPort("UV", PortType::Vec2);
        }

        void Evaluate() override
        {
            glm::vec2 uv(0.0f, 0.0f);
            SetOutput("UV", uv);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    class ColorNode : public TypedNode
    {
    public:
        ColorNode(NodeId id, const std::string& typeName, const glm::vec4& color = glm::vec4(1.0f))
            : TypedNode(typeName, id)
            , m_Color(color)
        {
            AddOutputPort("Color", PortType::Vec4);
        }

        const glm::vec4& GetColor() const { return m_Color; }
        void SetColor(const glm::vec4& c) { m_Color = c; }

        void Evaluate() override
        {
            SetOutput("Color", m_Color);
        }

        void SerializeProperties(YAML::Node& out) const override
        {
            YAML::Node arr;
            arr.push_back(m_Color.r);
            arr.push_back(m_Color.g);
            arr.push_back(m_Color.b);
            arr.push_back(m_Color.a);
            out["color"] = arr;
        }

        void DeserializeProperties(const YAML::Node& in) override
        {
            if (auto c = in["color"])
            {
                if (c.IsSequence() && c.size() >= 4)
                {
                    m_Color.r = c[0].as<float>();
                    m_Color.g = c[1].as<float>();
                    m_Color.b = c[2].as<float>();
                    m_Color.a = c[3].as<float>();
                }
            }
        }

    private:
        glm::vec4 m_Color;
    };

    class ColorMaskNode : public TypedNode
    {
    public:
        ColorMaskNode(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
            , m_UseR(true), m_UseG(true), m_UseB(true), m_UseA(true)
        {
            AddInputPort("Color", PortType::Vec4);
            AddInputPort("UseR", PortType::Bool);
            AddInputPort("UseG", PortType::Bool);
            AddInputPort("UseB", PortType::Bool);
            AddInputPort("UseA", PortType::Bool);
            AddOutputPort("Color", PortType::Vec4);
        }

        void Evaluate() override
        {
            glm::vec4 c(0.0f);
            try { c = std::any_cast<glm::vec4>(GetInputPortValue("Color")); }
            catch (...) {}

            bool useR = m_UseR;
            bool useG = m_UseG;
            bool useB = m_UseB;
            bool useA = m_UseA;

            try { useR = std::any_cast<bool>(GetInputPortValue("UseR")); }
            catch (...) {}
            try { useG = std::any_cast<bool>(GetInputPortValue("UseG")); }
            catch (...) {}
            try { useB = std::any_cast<bool>(GetInputPortValue("UseB")); }
            catch (...) {}
            try { useA = std::any_cast<bool>(GetInputPortValue("UseA")); }
            catch (...) {}

            glm::vec4 outColor(
                useR ? c.r : 0.0f,
                useG ? c.g : 0.0f,
                useB ? c.b : 0.0f,
                useA ? c.a : 0.0f
            );

            SetOutput("Color", outColor);
        }

        bool GetUseR() const { return m_UseR; }
        bool GetUseG() const { return m_UseG; }
        bool GetUseB() const { return m_UseB; }
        bool GetUseA() const { return m_UseA; }

        void SetUseR(bool v) { m_UseR = v; }
        void SetUseG(bool v) { m_UseG = v; }
        void SetUseB(bool v) { m_UseB = v; }
        void SetUseA(bool v) { m_UseA = v; }

        void SerializeProperties(YAML::Node& out) const override
        {
            out["useR"] = m_UseR;
            out["useG"] = m_UseG;
            out["useB"] = m_UseB;
            out["useA"] = m_UseA;
        }

        void DeserializeProperties(const YAML::Node& in) override
        {
            if (auto n = in["useR"]) m_UseR = n.as<bool>();
            if (auto n = in["useG"]) m_UseG = n.as<bool>();
            if (auto n = in["useB"]) m_UseB = n.as<bool>();
            if (auto n = in["useA"]) m_UseA = n.as<bool>();
        }

    private:
        bool m_UseR, m_UseG, m_UseB, m_UseA;
    };
}