#include "SandboxLayer.h"

#include "GraphicsPanel.h"

#include <quasar/core/Application.h>
#include <quasar/events/Event.h>
#include <quasar/events/EventDispatcher.h>
#include <quasar/events/KeyEvent.h>
#include <quasar/events/MouseEvent.h>
#include <quasar/events/WindowEvent.h>
#include <quasar/input/GamepadCodes.h>
#include <quasar/input/Input.h>
#include <quasar/input/KeyCodes.h>
#include <quasar/input/MouseCodes.h>
#include <quasar/io/EngineResources.h>
#include <quasar/log/Log.h>
#include <quasar/renderer/Bloom.h>
#include <quasar/renderer/FrameProfiler.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Lights.h>
#include <quasar/renderer/PipelineLibrary.h>
#include <quasar/renderer/PostProcessing.h>
#include <quasar/renderer/Primitives.h>
#include <quasar/scene/SceneRenderer.h>
#include <quasar/scene/components/CameraComponent.h>
#include <quasar/scene/components/DirectionalLightComponent.h>
#include <quasar/scene/components/MeshRendererComponent.h>
#include <quasar/scene/components/PointLightComponent.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>
#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <utility>
#include <vector>

namespace Quasar::Sandbox
{
    namespace
    {
        constexpr glm::vec4 White{1.0f};
        constexpr glm::vec3 SkyColor{0.45f, 0.62f, 0.85f};
        constexpr float StickLookSpeed = 1500.0f;
        constexpr GamepadId Gamepad{0};
        constexpr std::array<std::size_t, 6> CubeSides{5, 10, 15, 20, 25, 30};
        constexpr float GroundMargin = 10.0f;
        constexpr float GroundTileSize = 2.0f;
        constexpr float BulbSize = 0.25f;
        constexpr float BulbBrightness = 3.0f;
        constexpr float LightIntensity = 6.0f;
        constexpr std::size_t GapsPerLight = 5;
        constexpr std::size_t MaxLightsPerSide = 3;
        constexpr std::array<glm::vec3, 6> LightColors{glm::vec3(1.0f, 0.6f, 0.3f), glm::vec3(0.3f, 0.5f, 1.0f), glm::vec3(0.4f, 1.0f, 0.4f),
                                                       glm::vec3(1.0f, 0.3f, 0.8f), glm::vec3(1.0f, 0.9f, 0.3f), glm::vec3(0.3f, 1.0f, 1.0f)};

        static_assert(MaxLightsPerSide * MaxLightsPerSide * MaxLightsPerSide <= MaxPointLights);

        constexpr glm::vec3 CameraStart{-24.0f, 24.0f, 32.0f};
        constexpr float CameraStartYaw = -37.0f;
        constexpr float CameraStartPitch = -20.0f;

        struct SpinComponent
        {
            float Speed = 0.0f;
        };

        std::string FormatVector(std::string_view name, glm::vec3 vector)
        {
            return fmt::format("{} {:.2f}, {:.2f}, {:.2f}", name, vector.x, vector.y, vector.z);
        }
    }

    SandboxLayer::SandboxLayer() : Layer("Sandbox") {}

    void SandboxLayer::OnAttach()
    {
        const Window& window = Application::Get().GetWindow();
        m_Ready = CreateScene();
        if (!m_Ready)
        {
            QUASAR_ERROR("The sandbox scene cannot be created");
        }
        m_Scene.SetViewportSize(window.GetWidth(), window.GetHeight());

        if (SceneRenderer* renderer = Application::Get().GetSceneRenderer(); renderer != nullptr)
        {
            SceneRendererSettings settings{.Background = SkyColor};
            settings.Shadows.Distance = 60.0f;
            renderer->SetSettings(settings);
            renderer->GetPostProcessing().Add(Bloom::Create(*window.GetGraphicsContext()));
        }
        if (GraphicsContext* context = window.GetGraphicsContext(); context != nullptr)
        {
            context->GetProfiler().SetEnabled(true);
        }
    }

    void SandboxLayer::OnDetach()
    {
        SetLooking(false);
        if (GraphicsContext* context = Application::Get().GetWindow().GetGraphicsContext(); context != nullptr)
        {
            context->GetProfiler().SetEnabled(false);
        }
    }

    void SandboxLayer::OnUpdate(Timestep timestep)
    {
        m_Performance.Record(timestep);

        if (m_Looking && !Input::IsMouseButtonPressed(MouseButton::Right))
        {
            SetLooking(false);
        }

        FlyCameraInput input{.Move = {Input::GetKeyAxis(Key::A, Key::D), Input::GetKeyAxis(Key::LeftControl, Key::Space), Input::GetKeyAxis(Key::S, Key::W)},
                             .Fast = Input::IsKeyPressed(Key::LeftShift)};
        if (m_Looking)
        {
            input.Look = Input::GetMouseDelta();
        }
        if (Input::IsGamepadConnected(Gamepad))
        {
            const auto axis = [](GamepadAxis value) { return Input::GetGamepadAxis(Gamepad, value); };
            input.Move += glm::vec3(axis(GamepadAxis::LeftX), axis(GamepadAxis::RightTrigger) - axis(GamepadAxis::LeftTrigger), -axis(GamepadAxis::LeftY));
            input.Look += glm::vec2(axis(GamepadAxis::RightX), axis(GamepadAxis::RightY)) * StickLookSpeed * timestep.GetSeconds();
            input.Fast = input.Fast || Input::IsGamepadButtonPressed(Gamepad, GamepadButton::LeftThumb);
        }
        if (m_Camera.IsValid())
        {
            m_CameraController.Update(timestep, input, m_Camera.Get<TransformComponent>());
        }

        m_Scene.ForEach<TransformComponent, SpinComponent>(
            [timestep](Entity /*entity*/, TransformComponent& transform, const SpinComponent& spin)
            { transform.Rotation = glm::angleAxis(spin.Speed * timestep.GetSeconds(), glm::vec3(0.0f, 1.0f, 0.0f)) * transform.Rotation; });
    }

    void SandboxLayer::OnRender()
    {
        SceneRenderer* renderer = Application::Get().GetSceneRenderer();
        if (m_Ready && renderer != nullptr)
        {
            renderer->Render(m_Scene);
        }
    }

    void SandboxLayer::OnImGuiRender()
    {
        m_Performance.Draw(Application::Get().GetWindow().GetGraphicsContext(), Application::Get().GetSceneRenderer());
        DrawScenePanel();
        DrawGraphicsPanel();
    }

    void SandboxLayer::DrawScenePanel()
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        const ImGuiCond placement = ImGui::GetFrameCount() < 3 ? ImGuiCond_Always : ImGuiCond_FirstUseEver;
        ImGui::SetNextWindowPos({viewport->WorkPos.x + (viewport->WorkSize.x * 0.5f), viewport->WorkPos.y + viewport->WorkSize.y - 10.0f}, placement,
                                {0.5f, 1.0f});
        if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::SeparatorText("Cube of objects");
            const std::string label = fmt::format("{0}x{0}x{0}", CubeSides.at(static_cast<std::size_t>(m_CubeSize)));
            bool rebuild = false;
            if (ImGui::BeginCombo("Size", label.c_str()))
            {
                for (std::size_t index = 0; index < CubeSides.size(); ++index)
                {
                    const std::size_t side = CubeSides.at(index);
                    const bool selected = static_cast<std::size_t>(m_CubeSize) == index;
                    if (ImGui::Selectable(fmt::format("{0}x{0}x{0} ({1} objects)", side, side * side * side).c_str(), selected))
                    {
                        m_CubeSize = static_cast<int>(index);
                        rebuild = true;
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::SliderFloat("Spacing", &m_Spacing, 1.5f, 4.0f, "%.1f");
            rebuild = ImGui::IsItemDeactivatedAfterEdit() || rebuild;
            rebuild = ImGui::Checkbox("Spin the objects", &m_Spin) || rebuild;
            if (rebuild)
            {
                BuildCube();
            }
            ImGui::TextUnformatted(fmt::format("{} point lights, {} entities", m_LightCount, m_Scene.GetEntityCount()).c_str());

            ImGui::SeparatorText("Sun");
            const bool azimuthChanged = ImGui::SliderFloat("Azimuth", &m_SunAzimuth, -180.0f, 180.0f, "%.0f deg");
            const bool elevationChanged = ImGui::SliderFloat("Elevation", &m_SunElevation, 0.0f, 90.0f, "%.0f deg");
            if (azimuthChanged || elevationChanged)
            {
                UpdateSun();
            }
            if (m_Sun.IsValid())
            {
                ImGui::TextUnformatted(FormatVector("Direction", m_Sun.Get<TransformComponent>().GetForward()).c_str());
                auto& sun = m_Sun.Get<DirectionalLightComponent>();
                ImGui::SliderFloat("Intensity", &sun.Intensity, 0.0f, 2.0f, "%.2f");
                ImGui::SliderFloat("Ambient", &sun.Ambient, 0.0f, 1.0f, "%.2f");
            }

            ImGui::SeparatorText("Camera");
            if (m_Camera.IsValid())
            {
                const TransformComponent& camera = m_Camera.Get<TransformComponent>();
                ImGui::TextUnformatted(FormatVector("Position", camera.Position).c_str());
                ImGui::TextUnformatted(FormatVector("Direction", camera.GetForward()).c_str());
            }
        }
        ImGui::End();
    }

    void SandboxLayer::OnEvent(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowResizeEvent>(
            [this](const WindowResizeEvent& resize)
            {
                m_Scene.SetViewportSize(resize.GetWidth(), resize.GetHeight());
                return false;
            });
        dispatcher.Dispatch<KeyPressedEvent>(
            [](const KeyPressedEvent& pressed)
            {
                if (pressed.GetKey() != Key::F11 || pressed.IsRepeat())
                {
                    return false;
                }
                Window& window = Application::Get().GetWindow();
                window.SetFullscreen(!window.IsFullscreen());
                return true;
            });
        dispatcher.Dispatch<MouseButtonPressedEvent>(
            [this](const MouseButtonPressedEvent& pressed)
            {
                if (pressed.GetButton() != MouseButton::Right)
                {
                    return false;
                }
                SetLooking(true);
                return true;
            });
    }

    bool SandboxLayer::CreateScene()
    {
        Application& application = Application::Get();
        GraphicsContext* context = application.GetWindow().GetGraphicsContext();
        const PipelineLibrary* pipelines = application.GetPipelines();
        if (context == nullptr || pipelines == nullptr)
        {
            return false;
        }

        m_Cube = Mesh::Create(*context, Primitives::CreateCube());
        m_Sphere = Mesh::Create(*context, Primitives::CreateSphere());
        if (m_Cube == nullptr || m_Sphere == nullptr || !CreateMaterials(*context, *pipelines))
        {
            return false;
        }

        m_Ground = m_Scene.CreateEntity("Ground");
        m_Ground.Get<TransformComponent>().Rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        m_Ground.Add<MeshRendererComponent>(MeshRendererComponent{.Material = m_GroundMaterial});

        m_Sun = m_Scene.CreateEntity("Sun");
        m_Sun.Add<DirectionalLightComponent>(DirectionalLightComponent{.Ambient = 0.25f});
        UpdateSun();

        m_Camera = m_Scene.CreateEntity("Camera");
        m_Camera.Get<TransformComponent>().Position = CameraStart;
        m_Camera.Add<CameraComponent>().Camera.SetPerspective({.VerticalFov = glm::radians(60.0f), .Near = 0.1f, .Far = 500.0f});
        m_CameraController.SetRotation(CameraStartYaw, CameraStartPitch);

        BuildCube();
        return m_GroundMesh != nullptr;
    }

    bool SandboxLayer::CreateMaterials(GraphicsContext& context, const PipelineLibrary& pipelines)
    {
        const std::filesystem::path textures = GetResourcesDirectory() / "sandbox" / "textures";
        Texture2D* ground = AddTexture(context, textures / "test3.png");
        const std::array<Texture2D*, 2> objects{AddTexture(context, textures / "test.png"), AddTexture(context, textures / "test2.png")};

        m_GroundMaterial = AddMaterial(pipelines, PipelineLibrary::BlinnPhong, ground, {0.8f, 0.8f, 0.8f, 1.0f});
        if (m_GroundMaterial == nullptr)
        {
            return false;
        }
        m_GroundMaterial->SetFloat("Specular", 0.1f);

        for (std::size_t index = 0; index < m_ObjectMaterials.size(); ++index)
        {
            Material* material = AddMaterial(pipelines, PipelineLibrary::BlinnPhong, objects.at(index / 2), White);
            if (material == nullptr)
            {
                return false;
            }
            const bool shiny = index % 2 == 1;
            material->SetFloat("Specular", shiny ? 1.0f : 0.0f);
            material->SetFloat("Shininess", shiny ? 64.0f : 8.0f);
            m_ObjectMaterials.at(index) = material;
        }

        for (const glm::vec3& color : LightColors)
        {
            m_BulbMaterials.push_back(AddMaterial(pipelines, PipelineLibrary::Unlit, nullptr, glm::vec4(color * BulbBrightness, 1.0f)));
        }
        return std::ranges::none_of(m_BulbMaterials, [](const Material* bulb) { return bulb == nullptr; });
    }

    Texture2D* SandboxLayer::AddTexture(GraphicsContext& context, const std::filesystem::path& file)
    {
        Scope<Texture2D> texture = context.LoadTexture2D(file);
        if (texture == nullptr)
        {
            QUASAR_WARN("The sandbox uses white instead of '{}'", file.generic_string());
            return nullptr;
        }
        return m_Textures.emplace_back(std::move(texture)).get();
    }

    Material* SandboxLayer::AddMaterial(const PipelineLibrary& pipelines, std::string_view pipeline, Texture2D* texture, glm::vec4 color)
    {
        Scope<Material> material = pipelines.CreateMaterial(pipeline);
        if (material == nullptr)
        {
            return nullptr;
        }
        material->SetVec4("Color", color);
        material->SetTexture("Texture", texture);
        return m_Materials.emplace_back(std::move(material)).get();
    }

    Entity SandboxLayer::AddObject(std::string name, const Mesh& mesh, Material& material, const TransformComponent& transform)
    {
        Entity entity = m_Scene.CreateEntity(std::move(name));
        entity.Get<TransformComponent>() = transform;
        entity.Add<MeshRendererComponent>(MeshRendererComponent{.Mesh = &mesh, .Material = &material});
        m_CubeEntities.push_back(entity);
        return entity;
    }

    void SandboxLayer::BuildCube()
    {
        for (const Entity entity : m_CubeEntities)
        {
            m_Scene.DestroyEntity(entity);
        }
        m_CubeEntities.clear();

        const std::size_t side = CubeSides.at(static_cast<std::size_t>(m_CubeSize));
        const float start = -0.5f * m_Spacing * static_cast<float>(side - 1);
        m_CubeEntities.reserve((side * side * side) + (MaxLightsPerSide * MaxLightsPerSide * MaxLightsPerSide));
        for (std::size_t y = 0; y < side; ++y)
        {
            for (std::size_t z = 0; z < side; ++z)
            {
                for (std::size_t x = 0; x < side; ++x)
                {
                    const bool cube = (x + y + z) % 2 == 0;
                    Material& material = *m_ObjectMaterials.at(((z % 2) * 2) + (x % 2));
                    const glm::vec3 position(start + (m_Spacing * static_cast<float>(x)), m_Spacing * static_cast<float>(y + 1),
                                             start + (m_Spacing * static_cast<float>(z)));
                    Entity object = AddObject("Object", cube ? *m_Cube : *m_Sphere, material, {.Position = position});
                    if (m_Spin)
                    {
                        object.Add<SpinComponent>(SpinComponent{.Speed = 0.5f + (0.25f * static_cast<float>((x + y + z) % 5))});
                    }
                }
            }
        }
        PlaceLights(side, start);
        ResizeGround((m_Spacing * static_cast<float>(side)) + (2.0f * GroundMargin));
    }

    void SandboxLayer::PlaceLights(std::size_t side, float start)
    {
        m_LightCount = 0;
        if (side < 2)
        {
            return;
        }
        const std::size_t gaps = side - 1;
        const std::size_t count = std::clamp((gaps + GapsPerLight - 1) / GapsPerLight, std::size_t{1}, MaxLightsPerSide);
        const auto gapPosition = [this, gaps, count, start](std::size_t light)
        {
            const std::size_t gap = (((2 * light) + 1) * gaps) / (2 * count);
            return start + (m_Spacing * (static_cast<float>(gap) + 0.5f));
        };
        for (std::size_t y = 0; y < count; ++y)
        {
            for (std::size_t z = 0; z < count; ++z)
            {
                for (std::size_t x = 0; x < count; ++x)
                {
                    const std::size_t colorIndex = m_LightCount % LightColors.size();
                    const glm::vec3 position(gapPosition(x), gapPosition(y) - start + m_Spacing, gapPosition(z));
                    Entity bulb = AddObject("Point light", *m_Sphere, *m_BulbMaterials.at(colorIndex), {.Position = position, .Scale = glm::vec3(BulbSize)});
                    bulb.Get<MeshRendererComponent>().CastShadows = false;
                    bulb.Add<PointLightComponent>(
                        PointLightComponent{.Color = LightColors.at(colorIndex), .Intensity = LightIntensity, .Range = 3.0f * m_Spacing});
                    ++m_LightCount;
                }
            }
        }
    }

    void SandboxLayer::ResizeGround(float size)
    {
        GraphicsContext* context = Application::Get().GetWindow().GetGraphicsContext();
        if (context == nullptr)
        {
            return;
        }
        MeshData data = Primitives::CreateQuad();
        for (Vertex& vertex : data.Vertices)
        {
            vertex.TexCoord *= size / GroundTileSize;
        }
        Scope<Mesh> mesh = Mesh::Create(*context, data);
        if (mesh == nullptr)
        {
            return;
        }
        m_GroundMesh = std::move(mesh);
        m_Ground.Get<TransformComponent>().Scale = {size, size, 1.0f};
        m_Ground.Get<MeshRendererComponent>().Mesh = m_GroundMesh.get();
    }

    void SandboxLayer::UpdateSun()
    {
        if (m_Sun.IsValid())
        {
            m_Sun.Get<TransformComponent>().Rotation = glm::angleAxis(glm::radians(m_SunAzimuth), glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                       glm::angleAxis(glm::radians(-m_SunElevation), glm::vec3(1.0f, 0.0f, 0.0f));
        }
    }

    void SandboxLayer::SetLooking(bool looking)
    {
        if (looking != m_Looking)
        {
            m_Looking = looking;
            Application::Get().GetWindow().SetCursorCaptured(looking);
        }
    }
}
