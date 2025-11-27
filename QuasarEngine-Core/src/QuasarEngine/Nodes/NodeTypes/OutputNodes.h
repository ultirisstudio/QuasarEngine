#pragma once

#include <QuasarEngine/Nodes/Node.h>
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

#include <iostream>

namespace QuasarEngine
{
    class FloatOutputNode : public TypedNode
    {
    public:
        FloatOutputNode(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            SetIsOutputNode(true);
            AddInputPort("Value", PortType::Float);
        }

        void Evaluate() override
        {
            
        }
    };

    class Vec3OutputNode : public TypedNode
    {
    public:
        Vec3OutputNode(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            SetIsOutputNode(true);
            AddInputPort("Value", PortType::Vec3);
        }

        void Evaluate() override
        {
            
        }
    };

    class MaterialOutputNode : public TypedNode
    {
    public:
        MaterialOutputNode(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            SetIsOutputNode(true);

            AddInputPort("BaseColor", PortType::Vec3);
            AddInputPort("Metallic", PortType::Float);
            AddInputPort("Roughness", PortType::Float);
            AddInputPort("Emissive", PortType::Vec3);
            AddInputPort("Opacity", PortType::Float);
        }

        void Evaluate() override
        {
			
        }

        glm::vec3 GetBaseColor() const { return GetInput<glm::vec3>("BaseColor", glm::vec3(1.0f)); }
        float     GetMetallic() const { return GetInput<float>("Metallic", 0.0f); }
        float     GetRoughness() const { return GetInput<float>("Roughness", 0.5f); }
        glm::vec3 GetEmissive() const { return GetInput<glm::vec3>("Emissive", glm::vec3(0.0f)); }
        float     GetOpacity() const { return GetInput<float>("Opacity", 1.0f); }

        void SerializeProperties(YAML::Node& out) const override
        {
            
        }

        void DeserializeProperties(const YAML::Node& in) override
        {
            
        }
    };
}
