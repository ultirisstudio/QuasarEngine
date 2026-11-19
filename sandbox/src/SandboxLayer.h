#pragma once

#include "FlyCamera.h"

#include <quasar/core/Base.h>
#include <quasar/core/Layer.h>
#include <quasar/renderer/Material.h>
#include <quasar/renderer/Mesh.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/Renderer.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/Texture.h>
#include <quasar/scene/Entity.h>
#include <quasar/scene/Scene.h>
#include <quasar/scene/SceneRenderer.h>
#include <quasar/scene/components/TransformComponent.h>
#include <quasar/time/Timestep.h>

#include <glm/vec4.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Quasar
{
    class Event;
    class GraphicsContext;
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
        bool CreateScene(GraphicsContext& context);
        void AddObject(std::string name, const Mesh& mesh, Material& material, const TransformComponent& transform, float spin = 0.0f);
        static Scope<Texture2D> LoadTexture(GraphicsContext& context, const std::filesystem::path& file);
        Material* AddMaterial(GraphicsContext& context, Texture2D* texture, glm::vec4 color);
        void SetViewportSize(std::uint32_t width, std::uint32_t height);
        void SetLooking(bool looking);

        Scene m_Scene;
        Entity m_Camera;
        FlyCamera m_CameraController;
        Scope<Renderer> m_Renderer;
        Scope<SceneRenderer> m_SceneRenderer;
        Scope<Shader> m_Shader;
        Scope<Pipeline> m_Pipeline;
        Scope<Mesh> m_Cube;
        Scope<Mesh> m_Sphere;
        Scope<Mesh> m_Ground;
        std::vector<Scope<Texture2D>> m_Textures;
        std::vector<Scope<Material>> m_Materials;
        float m_FrameTime = 0.0f;
        bool m_Looking = false;
        bool m_VSync = true;
    };
}
