#pragma once

#include <quasar/core/Base.h>
#include <quasar/core/Layer.h>
#include <quasar/renderer/Material.h>
#include <quasar/renderer/Mesh.h>
#include <quasar/renderer/Texture.h>
#include <quasar/scene/Entity.h>
#include <quasar/scene/FlyCameraController.h>
#include <quasar/scene/Scene.h>
#include <quasar/scene/components/TransformComponent.h>
#include <quasar/time/Timestep.h>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace Quasar
{
    class Event;
    class GraphicsContext;
    class PipelineLibrary;
}

namespace Quasar::Sandbox
{
    class SandboxLayer final : public Layer
    {
    public:
        SandboxLayer();

        void OnAttach() override;
        void OnDetach() override;
        void OnUpdate(Timestep timestep) override;
        void OnRender() override;
        void OnImGuiRender() override;
        void OnEvent(Event& event) override;

    private:
        bool CreateScene();
        Entity AddObject(std::string name, const Mesh& mesh, Material& material, const TransformComponent& transform, float spin = 0.0f);
        Texture2D* AddTexture(GraphicsContext& context, const std::filesystem::path& file);
        Material* AddMaterial(const PipelineLibrary& pipelines, std::string_view pipeline, Texture2D* texture, glm::vec4 color);
        void UpdateSun();
        void SetLooking(bool looking);

        Scene m_Scene;
        Entity m_Camera;
        Entity m_Sun;
        FlyCameraController m_CameraController;
        Scope<Mesh> m_Cube;
        Scope<Mesh> m_Sphere;
        Scope<Mesh> m_Ground;
        std::vector<Scope<Texture2D>> m_Textures;
        std::vector<Scope<Material>> m_Materials;
        glm::vec2 m_SunAngles{30.0f, -50.0f};
        float m_FrameTime = 0.0f;
        bool m_Ready = false;
        bool m_Looking = false;
        bool m_VSync = true;
    };
}
