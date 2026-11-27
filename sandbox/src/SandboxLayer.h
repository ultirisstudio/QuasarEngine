#pragma once

#include "PerformancePanel.h"

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

#include <array>
#include <cstddef>
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
        bool CreateMaterials(GraphicsContext& context, const PipelineLibrary& pipelines);
        Texture2D* AddTexture(GraphicsContext& context, const std::filesystem::path& file);
        Material* AddMaterial(const PipelineLibrary& pipelines, std::string_view pipeline, Texture2D* texture, glm::vec4 color);
        Entity AddObject(std::string name, const Mesh& mesh, Material& material, const TransformComponent& transform);
        void BuildCube();
        void PlaceLights(std::size_t side, float start);
        void ResizeGround(float size);
        void ResetCamera();
        void DrawScenePanel();
        void UpdateSun();
        void SetLooking(bool looking);

        Scene m_Scene;
        Entity m_Camera;
        Entity m_Sun;
        Entity m_Ground;
        FlyCameraController m_CameraController;
        PerformancePanel m_Performance;
        Scope<Mesh> m_Cube;
        Scope<Mesh> m_Sphere;
        Scope<Mesh> m_GroundMesh;
        std::vector<Scope<Texture2D>> m_Textures;
        std::vector<Scope<Material>> m_Materials;
        Material* m_GroundMaterial = nullptr;
        std::array<Material*, 4> m_ObjectMaterials{};
        std::vector<Material*> m_BulbMaterials;
        std::vector<Entity> m_CubeEntities;
        std::size_t m_LightCount = 0;
        int m_CubeSize = 1;
        float m_Spacing = 2.0f;
        bool m_Spin = false;
        glm::vec2 m_SunAngles{30.0f, -50.0f};
        bool m_Ready = false;
        bool m_Looking = false;
    };
}
