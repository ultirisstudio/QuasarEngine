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
#include <quasar/time/Timestep.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <filesystem>
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
        struct SceneObject
        {
            const Mesh* Shape = nullptr;
            Material* Look = nullptr;
            glm::vec3 Position{0.0f};
            glm::vec3 Scale{1.0f};
            glm::quat Orientation{1.0f, 0.0f, 0.0f, 0.0f};
            float Spin = 0.0f;
        };

        bool CreateScene(GraphicsContext& context);
        static Scope<Texture2D> LoadTexture(GraphicsContext& context, const std::filesystem::path& file);
        Material* AddMaterial(GraphicsContext& context, Texture2D* texture, glm::vec4 color);
        void SetLooking(bool looking);

        FlyCamera m_Camera;
        Scope<Renderer> m_Renderer;
        Scope<Shader> m_Shader;
        Scope<Pipeline> m_Pipeline;
        Scope<Mesh> m_Cube;
        Scope<Mesh> m_Sphere;
        Scope<Mesh> m_Ground;
        std::vector<Scope<Texture2D>> m_Textures;
        std::vector<Scope<Material>> m_Materials;
        std::vector<SceneObject> m_Objects;

        float m_Time = 0.0f;
        float m_FrameTime = 0.0f;
        bool m_Looking = false;
        bool m_VSync = true;
    };
}
