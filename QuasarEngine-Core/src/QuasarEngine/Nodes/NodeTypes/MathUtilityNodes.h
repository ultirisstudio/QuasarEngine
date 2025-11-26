#pragma once

#include <QuasarEngine/Nodes/Node.h>
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>

namespace QuasarEngine
{
    class ClampFloatNode : public TypedNode
    {
    public:
        ClampFloatNode(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("Value", PortType::Float);
            AddInputPort("Min", PortType::Float);
            AddInputPort("Max", PortType::Float);
            AddOutputPort("Result", PortType::Float);
        }

        void Evaluate() override
        {
            float value = 0.0f;
            float minV = 0.0f;
            float maxV = 1.0f;

            try { value = std::any_cast<float>(GetInputPortValue("Value")); }
            catch (...) {}
            try { minV = std::any_cast<float>(GetInputPortValue("Min")); }
            catch (...) {}
            try { maxV = std::any_cast<float>(GetInputPortValue("Max")); }
            catch (...) {}

            if (minV > maxV)
                std::swap(minV, maxV);

            float r = std::clamp(value, minV, maxV);
            SetOutput("Result", r);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    class ClampVec3Node : public TypedNode
    {
    public:
        ClampVec3Node(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("Value", PortType::Vec3);
            AddInputPort("Min", PortType::Vec3);
            AddInputPort("Max", PortType::Vec3);
            AddOutputPort("Result", PortType::Vec3);
        }

        void Evaluate() override
        {
            glm::vec3 value(0.0f);
            glm::vec3 minV(0.0f);
            glm::vec3 maxV(1.0f);

            try { value = std::any_cast<glm::vec3>(GetInputPortValue("Value")); }
            catch (...) {}
            try { minV = std::any_cast<glm::vec3>(GetInputPortValue("Min")); }
            catch (...) {}
            try { maxV = std::any_cast<glm::vec3>(GetInputPortValue("Max")); }
            catch (...) {}

            glm::vec3 r;
            r.x = std::clamp(value.x, std::min(minV.x, maxV.x), std::max(minV.x, maxV.x));
            r.y = std::clamp(value.y, std::min(minV.y, maxV.y), std::max(minV.y, maxV.y));
            r.z = std::clamp(value.z, std::min(minV.z, maxV.z), std::max(minV.z, maxV.z));

            SetOutput("Result", r);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    class LerpFloatNode : public TypedNode
    {
    public:
        LerpFloatNode(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("A", PortType::Float);
            AddInputPort("B", PortType::Float);
            AddInputPort("T", PortType::Float);
            AddOutputPort("Result", PortType::Float);
        }

        void Evaluate() override
        {
            float a = 0.0f;
            float b = 0.0f;
            float t = 0.0f;

            try { a = std::any_cast<float>(GetInputPortValue("A")); }
            catch (...) {}
            try { b = std::any_cast<float>(GetInputPortValue("B")); }
            catch (...) {}
            try { t = std::any_cast<float>(GetInputPortValue("T")); }
            catch (...) {}

            float r = a + (b - a) * t;
            SetOutput("Result", r);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    class LerpVec3Node : public TypedNode
    {
    public:
        LerpVec3Node(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("A", PortType::Vec3);
            AddInputPort("B", PortType::Vec3);
            AddInputPort("T", PortType::Float);
            AddOutputPort("Result", PortType::Vec3);
        }

        void Evaluate() override
        {
            glm::vec3 a(0.0f);
            glm::vec3 b(0.0f);
            float t = 0.0f;

            try { a = std::any_cast<glm::vec3>(GetInputPortValue("A")); }
            catch (...) {}
            try { b = std::any_cast<glm::vec3>(GetInputPortValue("B")); }
            catch (...) {}
            try { t = std::any_cast<float>(GetInputPortValue("T")); }
            catch (...) {}

            glm::vec3 r = a + (b - a) * t;
            SetOutput("Result", r);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    class LengthVec2Node : public TypedNode
    {
    public:
        LengthVec2Node(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("Vector", PortType::Vec2);
            AddOutputPort("Length", PortType::Float);
        }

        void Evaluate() override
        {
            glm::vec2 v(0.0f);
            try { v = std::any_cast<glm::vec2>(GetInputPortValue("Vector")); }
            catch (...) {}
            float len = glm::length(v);
            SetOutput("Length", len);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    class LengthVec3Node : public TypedNode
    {
    public:
        LengthVec3Node(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("Vector", PortType::Vec3);
            AddOutputPort("Length", PortType::Float);
        }

        void Evaluate() override
        {
            glm::vec3 v(0.0f);
            try { v = std::any_cast<glm::vec3>(GetInputPortValue("Vector")); }
            catch (...) {}
            float len = glm::length(v);
            SetOutput("Length", len);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    class LengthVec4Node : public TypedNode
    {
    public:
        LengthVec4Node(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("Vector", PortType::Vec4);
            AddOutputPort("Length", PortType::Float);
        }

        void Evaluate() override
        {
            glm::vec4 v(0.0f);
            try { v = std::any_cast<glm::vec4>(GetInputPortValue("Vector")); }
            catch (...) {}
            float len = glm::length(v);
            SetOutput("Length", len);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    class NormalizeVec2Node : public TypedNode
    {
    public:
        NormalizeVec2Node(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("Vector", PortType::Vec2);
            AddOutputPort("Result", PortType::Vec2);
        }

        void Evaluate() override
        {
            glm::vec2 v(0.0f);
            try { v = std::any_cast<glm::vec2>(GetInputPortValue("Vector")); }
            catch (...) {}
            float len = glm::length(v);
            glm::vec2 r = (len > 0.0f) ? v / len : glm::vec2(0.0f);
            SetOutput("Result", r);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    class NormalizeVec3Node : public TypedNode
    {
    public:
        NormalizeVec3Node(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("Vector", PortType::Vec3);
            AddOutputPort("Result", PortType::Vec3);
        }

        void Evaluate() override
        {
            glm::vec3 v(0.0f);
            try { v = std::any_cast<glm::vec3>(GetInputPortValue("Vector")); }
            catch (...) {}
            float len = glm::length(v);
            glm::vec3 r = (len > 0.0f) ? v / len : glm::vec3(0.0f);
            SetOutput("Result", r);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    class NormalizeVec4Node : public TypedNode
    {
    public:
        NormalizeVec4Node(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("Vector", PortType::Vec4);
            AddOutputPort("Result", PortType::Vec4);
        }

        void Evaluate() override
        {
            glm::vec4 v(0.0f);
            try { v = std::any_cast<glm::vec4>(GetInputPortValue("Vector")); }
            catch (...) {}
            float len = glm::length(v);
            glm::vec4 r = (len > 0.0f) ? v / len : glm::vec4(0.0f);
            SetOutput("Result", r);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    class DotVec2Node : public TypedNode
    {
    public:
        DotVec2Node(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("A", PortType::Vec2);
            AddInputPort("B", PortType::Vec2);
            AddOutputPort("Dot", PortType::Float);
        }

        void Evaluate() override
        {
            glm::vec2 a(0.0f), b(0.0f);
            try { a = std::any_cast<glm::vec2>(GetInputPortValue("A")); }
            catch (...) {}
            try { b = std::any_cast<glm::vec2>(GetInputPortValue("B")); }
            catch (...) {}
            float d = glm::dot(a, b);
            SetOutput("Dot", d);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    class DotVec3Node : public TypedNode
    {
    public:
        DotVec3Node(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("A", PortType::Vec3);
            AddInputPort("B", PortType::Vec3);
            AddOutputPort("Dot", PortType::Float);
        }

        void Evaluate() override
        {
            glm::vec3 a(0.0f), b(0.0f);
            try { a = std::any_cast<glm::vec3>(GetInputPortValue("A")); }
            catch (...) {}
            try { b = std::any_cast<glm::vec3>(GetInputPortValue("B")); }
            catch (...) {}
            float d = glm::dot(a, b);
            SetOutput("Dot", d);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    class DotVec4Node : public TypedNode
    {
    public:
        DotVec4Node(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("A", PortType::Vec4);
            AddInputPort("B", PortType::Vec4);
            AddOutputPort("Dot", PortType::Float);
        }

        void Evaluate() override
        {
            glm::vec4 a(0.0f), b(0.0f);
            try { a = std::any_cast<glm::vec4>(GetInputPortValue("A")); }
            catch (...) {}
            try { b = std::any_cast<glm::vec4>(GetInputPortValue("B")); }
            catch (...) {}
            float d = glm::dot(a, b);
            SetOutput("Dot", d);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };

    class CrossVec3Node : public TypedNode
    {
    public:
        CrossVec3Node(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("A", PortType::Vec3);
            AddInputPort("B", PortType::Vec3);
            AddOutputPort("Result", PortType::Vec3);
        }

        void Evaluate() override
        {
            glm::vec3 a(0.0f), b(0.0f);
            try { a = std::any_cast<glm::vec3>(GetInputPortValue("A")); }
            catch (...) {}
            try { b = std::any_cast<glm::vec3>(GetInputPortValue("B")); }
            catch (...) {}
            glm::vec3 r = glm::cross(a, b);
            SetOutput("Result", r);
        }

        void SerializeProperties(YAML::Node& out) const override {}
        void DeserializeProperties(const YAML::Node& in) override {}
    };
}