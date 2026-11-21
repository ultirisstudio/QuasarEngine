#include "SandboxLayer.h"

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
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/PipelineLibrary.h>
#include <quasar/renderer/PostProcessing.h>
#include <quasar/renderer/Primitives.h>
#include <quasar/scene/SceneRenderer.h>
#include <quasar/scene/components/CameraComponent.h>
#include <quasar/scene/components/DirectionalLightComponent.h>
#include <quasar/scene/components/MeshRendererComponent.h>
#include <quasar/scene/components/PointLightComponent.h>
#include <quasar/scene/components/SpotLightComponent.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>
#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <tuple>
#include <utility>
#include <vector>

namespace Quasar::Sandbox
{
    namespace
    {
        constexpr glm::vec4 White{1.0f};
        constexpr glm::vec3 SkyColor{0.45f, 0.62f, 0.85f};
        constexpr float GroundSize = 30.0f;
        constexpr float GroundTiles = 15.0f;
        constexpr float StickLookSpeed = 1500.0f;
        constexpr float BulbBrightness = 3.0f;
        constexpr GamepadId Gamepad{0};

        struct SpinComponent
        {
            float Speed = 0.0f;
        };

        std::string KeyLabel(Key key)
        {
            std::string name = Application::Get().GetWindow().GetKeyName(key);
            std::ranges::transform(name, name.begin(), [](unsigned char character) { return static_cast<char>(std::toupper(character)); });
            return name;
        }
        void DrawVideoOptions()
        {
            Window& window = Application::Get().GetWindow();
            ImGui::SeparatorText("Video");
            bool fullscreen = window.IsFullscreen();
            if (ImGui::Checkbox("Fullscreen", &fullscreen))
            {
                window.SetFullscreen(fullscreen);
            }
            bool vsync = window.IsVSync();
            if (ImGui::Checkbox("VSync", &vsync))
            {
                window.SetVSync(vsync);
            }
        }

        void DrawQualityOptions(SceneRendererSettings& settings)
        {
            ImGui::SeparatorText("Quality");
            constexpr std::array<const char*, 4> antialiasing{"Off", "2x", "4x", "8x"};
            int level = std::min(std::countr_zero(settings.Samples), 3);
            if (ImGui::Combo("Antialiasing", &level, antialiasing.data(), static_cast<int>(antialiasing.size())))
            {
                settings.Samples = 1U << static_cast<unsigned>(level);
            }
            ImGui::Checkbox("Shadows", &settings.Shadows.Enabled);
            constexpr std::array<const char*, 4> resolutions{"512", "1024", "2048", "4096"};
            int resolution = std::clamp(std::countr_zero(settings.Shadows.Resolution) - 9, 0, 3);
            if (ImGui::Combo("Shadow quality", &resolution, resolutions.data(), static_cast<int>(resolutions.size())))
            {
                settings.Shadows.Resolution = 512U << static_cast<unsigned>(resolution);
            }
            ImGui::SliderFloat("Shadow distance", &settings.Shadows.Distance, 5.0f, 100.0f, "%.0f");
            if (ImGui::TreeNode("Shadow biases"))
            {
                ImGui::SliderFloat("Depth bias", &settings.Shadows.DepthBias, 0.0f, 0.005f, "%.4f");
                ImGui::SliderFloat("Normal bias", &settings.Shadows.NormalBias, 0.0f, 4.0f, "%.2f");
                ImGui::TreePop();
            }

            ImGui::SeparatorText("Image");
            ImGui::SliderFloat("Exposure", &settings.Exposure, 0.1f, 4.0f, "%.2f");
            constexpr std::array<const char*, 3> tonemaps{"None", "Reinhard", "ACES"};
            int tonemap = static_cast<int>(settings.Tonemap);
            ImGui::Combo("Tonemapping", &tonemap, tonemaps.data(), static_cast<int>(tonemaps.size()));
            settings.Tonemap = static_cast<Tonemapping>(tonemap);
        }

        void DrawBloomOptions(Bloom& bloom)
        {
            ImGui::Indent();
            BloomSettings settings = bloom.GetSettings();
            ImGui::SliderFloat("Bloom intensity", &settings.Intensity, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Bloom threshold", &settings.Threshold, 0.0f, 4.0f, "%.2f");
            ImGui::SliderFloat("Bloom soft knee", &settings.SoftKnee, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Bloom radius", &settings.Radius, 0.0f, 2.0f, "%.2f");
            bloom.SetSettings(settings);
            ImGui::Unindent();
        }

        void DrawEffectOptions(const PostProcessStack& effects)
        {
            ImGui::SeparatorText("Effects");
            if (effects.GetEffects().empty())
            {
                ImGui::BeginDisabled();
                ImGui::TextUnformatted("No effects");
                ImGui::EndDisabled();
            }
            for (const Scope<PostProcessEffect>& effect : effects.GetEffects())
            {
                bool enabled = effect->IsEnabled();
                if (ImGui::Checkbox(effect->GetName().c_str(), &enabled))
                {
                    effect->SetEnabled(enabled);
                }
                if (auto* bloom = dynamic_cast<Bloom*>(effect.get()); bloom != nullptr && enabled)
                {
                    DrawBloomOptions(*bloom);
                }
            }
        }

        void DrawGraphicsPanel()
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos({viewport->WorkPos.x + viewport->WorkSize.x - 10.0f, viewport->WorkPos.y + 10.0f}, ImGuiCond_FirstUseEver, {1.0f, 0.0f});
            if (ImGui::Begin("Graphics", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                DrawVideoOptions();
                if (SceneRenderer* renderer = Application::Get().GetSceneRenderer(); renderer != nullptr)
                {
                    SceneRendererSettings settings = renderer->GetSettings();
                    DrawQualityOptions(settings);
                    DrawEffectOptions(renderer->GetPostProcessing());
                    renderer->SetSettings(settings);
                }
            }
            ImGui::End();
        }
    }

    SandboxLayer::SandboxLayer() : Layer("Sandbox") {}

    void SandboxLayer::OnAttach()
    {
        const Window& window = Application::Get().GetWindow();
        m_CameraController.SetRotation(0.0f, -10.0f);

        m_Ready = CreateScene();
        if (!m_Ready)
        {
            QUASAR_ERROR("The sandbox scene cannot be created");
        }
        m_Scene.SetViewportSize(window.GetWidth(), window.GetHeight());

        if (SceneRenderer* renderer = Application::Get().GetSceneRenderer(); renderer != nullptr)
        {
            renderer->SetSettings({.Background = SkyColor});
            renderer->GetPostProcessing().Add(Bloom::Create(*window.GetGraphicsContext()));
        }
    }

    void SandboxLayer::OnDetach()
    {
        SetLooking(false);
    }

    void SandboxLayer::OnUpdate(Timestep timestep)
    {
        m_FrameTime = timestep.GetMilliseconds();

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
        DrawScenePanel();
        DrawGraphicsPanel();
    }

    void SandboxLayer::DrawScenePanel()
    {
        ImGui::SetNextWindowPos({10.0f, 10.0f}, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::SeparatorText("Rendering");
            ImGui::TextUnformatted(fmt::format("{:.0f} frames per second ({:.2f} ms)", ImGui::GetIO().Framerate, m_FrameTime).c_str());
            if (const SceneRenderer* renderer = Application::Get().GetSceneRenderer(); renderer != nullptr)
            {
                const FramebufferSpecification& image = renderer->GetSceneFramebuffer().GetSpecification();
                ImGui::TextUnformatted(fmt::format("Image: {}x{}, {} samples per pixel", image.Width, image.Height, image.Samples).c_str());
                const FramebufferSpecification& shadows = renderer->GetShadowMap().GetSpecification();
                ImGui::TextUnformatted(fmt::format("Shadow map: {}x{}", shadows.Width, shadows.Height).c_str());
            }

            ImGui::SeparatorText("Scene");
            ImGui::TextUnformatted(fmt::format("{} entities", m_Scene.GetEntityCount()).c_str());
            if (m_Camera.IsValid())
            {
                const glm::vec3& position = m_Camera.Get<TransformComponent>().Position;
                ImGui::TextUnformatted(fmt::format("Camera: {:.1f}, {:.1f}, {:.1f}", position.x, position.y, position.z).c_str());
            }
            ImGui::TextUnformatted(fmt::format("Yaw {:.0f}, pitch {:.0f}", m_CameraController.GetYaw(), m_CameraController.GetPitch()).c_str());

            ImGui::SeparatorText("Sun");
            const bool yawChanged = ImGui::SliderFloat("Yaw", &m_SunAngles.x, -180.0f, 180.0f, "%.0f");
            const bool pitchChanged = ImGui::SliderFloat("Pitch", &m_SunAngles.y, -90.0f, 0.0f, "%.0f");
            if (yawChanged || pitchChanged)
            {
                UpdateSun();
            }
            if (m_Sun.IsValid())
            {
                ImGui::SliderFloat("Intensity", &m_Sun.Get<DirectionalLightComponent>().Intensity, 0.0f, 2.0f, "%.2f");
            }

            ImGui::SeparatorText("Controls");
            ImGui::TextUnformatted(fmt::format("Move: {} {} {} {}", KeyLabel(Key::W), KeyLabel(Key::A), KeyLabel(Key::S), KeyLabel(Key::D)).c_str());
            ImGui::TextUnformatted("Up and down: Space and Left Ctrl");
            ImGui::TextUnformatted("Faster: Left Shift");
            ImGui::TextUnformatted("Look around: hold the right mouse button");
            ImGui::TextUnformatted("Fullscreen: F11");
            ImGui::TextUnformatted("Gamepad: left stick to move, right stick to look");
            ImGui::TextUnformatted("Triggers to go up and down, left stick click to go faster");
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
        MeshData groundData = Primitives::CreateQuad();
        for (Vertex& vertex : groundData.Vertices)
        {
            vertex.TexCoord *= GroundTiles;
        }
        m_Ground = Mesh::Create(*context, groundData);
        if (m_Cube == nullptr || m_Sphere == nullptr || m_Ground == nullptr)
        {
            return false;
        }

        const std::filesystem::path textures = GetResourcesDirectory() / "sandbox" / "textures";
        Texture2D* crate = AddTexture(*context, textures / "test.png");
        Texture2D* ball = AddTexture(*context, textures / "test2.png");
        Texture2D* ground = AddTexture(*context, textures / "test3.png");

        const std::string_view lit = PipelineLibrary::BlinnPhong;
        Material* crateMaterial = AddMaterial(*pipelines, lit, crate, White);
        Material* redCrate = AddMaterial(*pipelines, lit, crate, {1.0f, 0.45f, 0.45f, 1.0f});
        Material* ballMaterial = AddMaterial(*pipelines, lit, ball, White);
        Material* greenBall = AddMaterial(*pipelines, lit, ball, {0.5f, 1.0f, 0.5f, 1.0f});
        Material* unlitBall = AddMaterial(*pipelines, PipelineLibrary::Unlit, ball, White);
        Material* groundMaterial = AddMaterial(*pipelines, lit, ground, {0.8f, 0.8f, 0.8f, 1.0f});
        if (crateMaterial == nullptr || redCrate == nullptr || ballMaterial == nullptr || greenBall == nullptr || unlitBall == nullptr ||
            groundMaterial == nullptr)
        {
            return false;
        }
        groundMaterial->SetFloat("Specular", 0.1f);

        const glm::quat flat = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        AddObject("Ground", *m_Ground, *groundMaterial, {.Rotation = flat, .Scale = {GroundSize, GroundSize, 1.0f}});
        const Entity bigCrate = AddObject("Big crate", *m_Cube, *crateMaterial, {.Position = {0.0f, 1.5f, 0.0f}, .Scale = glm::vec3(1.5f)}, 0.8f);
        AddObject("Moon", *m_Sphere, *greenBall, {.Position = {1.2f, 0.4f, 0.0f}, .Scale = glm::vec3(0.3f)}).SetParent(bigCrate, false);
        AddObject("Crate", *m_Cube, *crateMaterial, {.Position = {-3.0f, 0.5f, -1.0f}});
        AddObject("Red crate", *m_Cube, *redCrate, {.Position = {3.0f, 0.75f, -2.0f}, .Scale = glm::vec3(1.5f)}, -0.4f);
        AddObject("Stacked crate", *m_Cube, *crateMaterial, {.Position = {-3.0f, 1.5f, -1.0f}}, 1.5f);
        AddObject("Ball", *m_Sphere, *ballMaterial, {.Position = {-1.5f, 0.5f, 2.0f}}, 1.0f);
        AddObject("Green ball", *m_Sphere, *greenBall, {.Position = {2.0f, 1.0f, 2.0f}, .Scale = glm::vec3(2.0f)}, 0.3f);
        AddObject("Unlit ball", *m_Sphere, *unlitBall, {.Position = {0.0f, 0.75f, -5.0f}, .Scale = glm::vec3(1.5f)}, -0.6f);

        Entity rig = m_Scene.CreateEntity("Lamp rig");
        rig.Get<TransformComponent>().Position = {0.0f, 2.5f, 0.0f};
        rig.Add<SpinComponent>(SpinComponent{.Speed = 0.7f});
        for (const auto& [name, color, position] : {std::tuple{"Warm lamp", glm::vec3(1.0f, 0.6f, 0.3f), glm::vec3(4.0f, 0.0f, 0.0f)},
                                                    std::tuple{"Cold lamp", glm::vec3(0.3f, 0.5f, 1.0f), glm::vec3(-4.0f, 0.0f, 0.0f)}})
        {
            Material* bulb = AddMaterial(*pipelines, PipelineLibrary::Unlit, nullptr, glm::vec4(color * BulbBrightness, 1.0f));
            if (bulb == nullptr)
            {
                return false;
            }
            Entity lamp = AddObject(name, *m_Sphere, *bulb, {.Position = position, .Scale = glm::vec3(0.2f)});
            lamp.Add<PointLightComponent>(PointLightComponent{.Color = color, .Intensity = 8.0f, .Range = 8.0f});
            lamp.Get<MeshRendererComponent>().CastShadows = false;
            lamp.SetParent(rig, false);
        }

        Entity spot = m_Scene.CreateEntity("Spot");
        spot.Get<TransformComponent>() = {.Position = {3.0f, 5.0f, -2.0f}, .Rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f))};
        spot.Add<SpotLightComponent>(SpotLightComponent{.Intensity = 20.0f, .Range = 10.0f});

        m_Sun = m_Scene.CreateEntity("Sun");
        m_Sun.Add<DirectionalLightComponent>(DirectionalLightComponent{.Ambient = 0.25f});
        UpdateSun();

        m_Camera = m_Scene.CreateEntity("Camera");
        m_Camera.Get<TransformComponent>().Position = {0.0f, 2.0f, 8.0f};
        m_Camera.Add<CameraComponent>().Camera.SetPerspective({.VerticalFov = glm::radians(60.0f), .Near = 0.1f, .Far = 500.0f});
        return true;
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

    Entity SandboxLayer::AddObject(std::string name, const Mesh& mesh, Material& material, const TransformComponent& transform, float spin)
    {
        Entity entity = m_Scene.CreateEntity(std::move(name));
        entity.Get<TransformComponent>() = transform;
        entity.Add<MeshRendererComponent>(MeshRendererComponent{.Mesh = &mesh, .Material = &material});
        if (spin != 0.0f)
        {
            entity.Add<SpinComponent>(SpinComponent{.Speed = spin});
        }
        return entity;
    }

    void SandboxLayer::UpdateSun()
    {
        if (m_Sun.IsValid())
        {
            m_Sun.Get<TransformComponent>().Rotation = glm::angleAxis(glm::radians(m_SunAngles.x), glm::vec3(0.0f, 1.0f, 0.0f)) *
                                                       glm::angleAxis(glm::radians(m_SunAngles.y), glm::vec3(1.0f, 0.0f, 0.0f));
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
