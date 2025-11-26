#pragma once

#include <QuasarEngine/Nodes/Node.h>
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

namespace QuasarEngine
{
    class TextureSampleNode : public TypedNode
    {
    public:
        TextureSampleNode(NodeId id, const std::string& typeName)
            : TypedNode(typeName, id)
        {
            AddInputPort("UV", PortType::Vec2);
            AddOutputPort("Color", PortType::Vec4);
            AddOutputPort("R", PortType::Float);
            AddOutputPort("G", PortType::Float);
            AddOutputPort("B", PortType::Float);
            AddOutputPort("A", PortType::Float);
        }

        void SetRelativePath(const std::string& path) { m_RelativePath = path; }
        const std::string& GetRelativePath() const { return m_RelativePath; }

        void* GetImGuiTextureId() const { return m_ImGuiTextureId; }
        void  SetImGuiTextureId(void* id) { m_ImGuiTextureId = id; }

        void Evaluate() override
        {
            glm::vec2 uv(0.0f);
            try { uv = std::any_cast<glm::vec2>(GetInputPortValue("UV")); }
            catch (...) {}

            glm::vec4 color(uv.x, uv.y, 0.0f, 1.0f);

            SetOutput("Color", color);
            SetOutput("R", color.r);
            SetOutput("G", color.g);
            SetOutput("B", color.b);
            SetOutput("A", color.a);
        }

        void SerializeProperties(YAML::Node& out) const override
        {
            out["texturePath"] = m_RelativePath;
        }

        void DeserializeProperties(const YAML::Node& in) override
        {
            if (auto n = in["texturePath"])
                m_RelativePath = n.as<std::string>();
        }

    private:
        std::string m_RelativePath;
        void* m_ImGuiTextureId = nullptr;
    };
}