#pragma once

#include <QuasarEngine/Nodes/Node.h>
#include <glm/glm.hpp>

namespace QuasarEngine
{
    class Vec2ComponentsNode : public TypedNode
    {
    public:
        Vec2ComponentsNode(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("Vector", PortType::Vec2);
            AddOutputPort("X", PortType::Float);
            AddOutputPort("Y", PortType::Float);
        }

        void Evaluate() override
        {
            glm::vec2 v(0.0f);
            try { v = std::any_cast<glm::vec2>(GetInputPortValue("Vector")); }
            catch (...) {}

            SetOutput("X", v.x);
            SetOutput("Y", v.y);
        }
    };

    class Vec3ComponentsNode : public TypedNode
    {
    public:
        Vec3ComponentsNode(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("Vector", PortType::Vec3);
            AddOutputPort("X", PortType::Float);
            AddOutputPort("Y", PortType::Float);
            AddOutputPort("Z", PortType::Float);
        }

        void Evaluate() override
        {
            glm::vec3 v(0.0f);
            try { v = std::any_cast<glm::vec3>(GetInputPortValue("Vector")); }
            catch (...) {}

            SetOutput("X", v.x);
            SetOutput("Y", v.y);
            SetOutput("Z", v.z);
        }
    };

    class Vec4ComponentsNode : public TypedNode
    {
    public:
        Vec4ComponentsNode(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("Vector", PortType::Vec4);
            AddOutputPort("X", PortType::Float);
            AddOutputPort("Y", PortType::Float);
            AddOutputPort("Z", PortType::Float);
            AddOutputPort("W", PortType::Float);
        }

        void Evaluate() override
        {
            glm::vec4 v(0.0f);
            try { v = std::any_cast<glm::vec4>(GetInputPortValue("Vector")); }
            catch (...) {}

            SetOutput("X", v.x);
            SetOutput("Y", v.y);
            SetOutput("Z", v.z);
            SetOutput("W", v.w);
        }
    };
}