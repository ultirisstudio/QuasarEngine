#include "SandboxLayer.h"

#include <quasar/core/Application.h>
#include <quasar/events/Event.h>
#include <quasar/events/EventDispatcher.h>
#include <quasar/events/MouseEvent.h>
#include <quasar/events/WindowEvent.h>
#include <quasar/input/Input.h>
#include <quasar/input/KeyCodes.h>
#include <quasar/input/MouseCodes.h>
#include <quasar/io/EngineResources.h>
#include <quasar/log/Log.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Image.h>
#include <quasar/renderer/Primitives.h>
#include <quasar/scene/components/CameraComponent.h>
#include <quasar/scene/components/MeshRendererComponent.h>
#include <quasar/scene/components/TransformComponent.h>

#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>
#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include <utility>

namespace Quasar::Sandbox
{
    namespace
    {
        constexpr glm::vec4 SkyColor{0.45f, 0.62f, 0.85f, 1.0f};
        constexpr glm::vec4 White{1.0f};
        constexpr float GroundSize = 30.0f;
        constexpr float GroundTiles = 15.0f;

        struct SpinComponent
        {
            float Speed = 0.0f;
        };

        float Axis(Key positive, Key negative)
        {
            return (Input::IsKeyPressed(positive) ? 1.0f : 0.0f) - (Input::IsKeyPressed(negative) ? 1.0f : 0.0f);
        }

        std::string KeyLabel(Key key)
        {
            std::string name = Application::Get().GetWindow().GetKeyName(key);
            std::ranges::transform(name, name.begin(), [](unsigned char character) { return static_cast<char>(std::toupper(character)); });
            return name;
        }
    }

    SandboxLayer::SandboxLayer() : Layer("Sandbox") {}

    void SandboxLayer::OnAttach()
    {
        Window& window = Application::Get().GetWindow();
        window.SetVSync(m_VSync);

        GraphicsContext* context = window.GetGraphicsContext();
        if (context == nullptr || !CreateScene(*context))
        {
            QUASAR_ERROR("The sandbox scene cannot be created");
            m_SceneRenderer.reset();
        }
        SetViewportSize(window.GetWidth(), window.GetHeight());
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

        FlyCameraInput input;
        if (m_Looking || !ImGui::GetIO().WantCaptureKeyboard)
        {
            input.Move = {Axis(Key::D, Key::A), Axis(Key::Space, Key::LeftControl), Axis(Key::W, Key::S)};
            input.Fast = Input::IsKeyPressed(Key::LeftShift);
        }
        if (m_Looking)
        {
            input.Look = Input::GetMouseDelta();
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
        if (m_SceneRenderer == nullptr)
        {
            return;
        }

        Application::Get().GetWindow().GetGraphicsContext()->ClearWindow(SkyColor);
        m_SceneRenderer->Render(m_Scene);
    }

    void SandboxLayer::OnImGuiRender()
    {
        ImGui::SetNextWindowPos({10.0f, 10.0f}, ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(fmt::format("{:.0f} frames per second ({:.2f} ms)", ImGui::GetIO().Framerate, m_FrameTime).c_str());
            ImGui::TextUnformatted(fmt::format("{} entities", m_Scene.GetEntityCount()).c_str());
            if (m_Camera.IsValid())
            {
                const glm::vec3& position = m_Camera.Get<TransformComponent>().Position;
                ImGui::TextUnformatted(fmt::format("Camera: {:.1f}, {:.1f}, {:.1f}", position.x, position.y, position.z).c_str());
            }
            ImGui::TextUnformatted(fmt::format("Yaw {:.0f}, pitch {:.0f}", m_CameraController.GetYaw(), m_CameraController.GetPitch()).c_str());
            if (ImGui::Checkbox("VSync", &m_VSync))
            {
                Application::Get().GetWindow().SetVSync(m_VSync);
            }

            ImGui::SeparatorText("Controls");
            ImGui::TextUnformatted(fmt::format("Move: {} {} {} {}", KeyLabel(Key::W), KeyLabel(Key::A), KeyLabel(Key::S), KeyLabel(Key::D)).c_str());
            ImGui::TextUnformatted("Up and down: Space and Left Ctrl");
            ImGui::TextUnformatted("Faster: Left Shift");
            ImGui::TextUnformatted("Look around: hold the right mouse button");
        }
        ImGui::End();
    }

    void SandboxLayer::OnEvent(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowResizeEvent>(
            [this](const WindowResizeEvent& resize)
            {
                SetViewportSize(resize.GetWidth(), resize.GetHeight());
                return false;
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

    bool SandboxLayer::CreateScene(GraphicsContext& context)
    {
        m_Renderer = Renderer::Create(context);
        if (m_Renderer != nullptr)
        {
            m_SceneRenderer = CreateScope<SceneRenderer>(*m_Renderer);
        }
        const std::optional<ShaderSpecification> unlit = ShaderSpecification::FromFile(GetEngineResourcesDirectory() / "shaders" / "Unlit.glsl");
        if (m_Renderer == nullptr || !unlit)
        {
            return false;
        }
        m_Shader = context.CreateShader(*unlit);
        if (m_Shader == nullptr)
        {
            return false;
        }
        m_Pipeline = context.CreatePipeline({.Shader = m_Shader.get()});
        m_Cube = Mesh::Create(context, Primitives::CreateCube());
        m_Sphere = Mesh::Create(context, Primitives::CreateSphere());
        MeshData groundData = Primitives::CreateQuad();
        for (Vertex& vertex : groundData.Vertices)
        {
            vertex.TexCoord *= GroundTiles;
        }
        m_Ground = Mesh::Create(context, groundData);
        if (m_Pipeline == nullptr || m_Cube == nullptr || m_Sphere == nullptr || m_Ground == nullptr)
        {
            return false;
        }

        const std::filesystem::path textures = GetResourcesDirectory() / "sandbox" / "textures";
        Texture2D* crate = m_Textures.emplace_back(LoadTexture(context, textures / "test.png")).get();
        Texture2D* ball = m_Textures.emplace_back(LoadTexture(context, textures / "test2.png")).get();
        Texture2D* ground = m_Textures.emplace_back(LoadTexture(context, textures / "test3.png")).get();

        Material* crateMaterial = AddMaterial(context, crate, White);
        Material* redCrate = AddMaterial(context, crate, {1.0f, 0.45f, 0.45f, 1.0f});
        Material* ballMaterial = AddMaterial(context, ball, White);
        Material* greenBall = AddMaterial(context, ball, {0.5f, 1.0f, 0.5f, 1.0f});
        Material* groundMaterial = AddMaterial(context, ground, {0.8f, 0.8f, 0.8f, 1.0f});
        if (crateMaterial == nullptr || redCrate == nullptr || ballMaterial == nullptr || greenBall == nullptr || groundMaterial == nullptr)
        {
            return false;
        }

        const glm::quat flat = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        AddObject("Ground", *m_Ground, *groundMaterial, {.Rotation = flat, .Scale = {GroundSize, GroundSize, 1.0f}});
        AddObject("Big crate", *m_Cube, *crateMaterial, {.Position = {0.0f, 1.5f, 0.0f}, .Scale = glm::vec3(1.5f)}, 0.8f);
        AddObject("Crate", *m_Cube, *crateMaterial, {.Position = {-3.0f, 0.5f, -1.0f}});
        AddObject("Red crate", *m_Cube, *redCrate, {.Position = {3.0f, 0.75f, -2.0f}, .Scale = glm::vec3(1.5f)}, -0.4f);
        AddObject("Stacked crate", *m_Cube, *crateMaterial, {.Position = {-3.0f, 1.5f, -1.0f}}, 1.5f);
        AddObject("Ball", *m_Sphere, *ballMaterial, {.Position = {-1.5f, 0.5f, 2.0f}}, 1.0f);
        AddObject("Green ball", *m_Sphere, *greenBall, {.Position = {2.0f, 1.0f, 2.0f}, .Scale = glm::vec3(2.0f)}, 0.3f);
        AddObject("Far ball", *m_Sphere, *ballMaterial, {.Position = {0.0f, 0.75f, -5.0f}, .Scale = glm::vec3(1.5f)}, -0.6f);

        m_Camera = m_Scene.CreateEntity("Camera");
        m_Camera.Get<TransformComponent>().Position = {0.0f, 2.0f, 8.0f};
        m_Camera.Add<CameraComponent>().Camera.SetPerspective({.VerticalFov = glm::radians(60.0f), .Near = 0.1f, .Far = 500.0f});
        return true;
    }

    Scope<Texture2D> SandboxLayer::LoadTexture(GraphicsContext& context, const std::filesystem::path& file)
    {
        const std::optional<Image> image = Image::FromFile(file);
        if (!image)
        {
            QUASAR_WARN("The sandbox uses white instead of '{}'", file.generic_string());
            return nullptr;
        }
        return context.CreateTexture2D({.Width = image->Width, .Height = image->Height}, image->Pixels);
    }

    Material* SandboxLayer::AddMaterial(GraphicsContext& context, Texture2D* texture, glm::vec4 color)
    {
        Scope<Material> material =
            Material::Create(context, {.Pipeline = m_Pipeline.get(), .Uniforms = {{"Color", UniformType::Vec4}}, .Textures = {{.Name = "Texture", .Slot = 0}}});
        if (material == nullptr)
        {
            return nullptr;
        }
        material->SetVec4("Color", color);
        material->SetTexture("Texture", texture);
        return m_Materials.emplace_back(std::move(material)).get();
    }

    void SandboxLayer::AddObject(std::string name, const Mesh& mesh, Material& material, const TransformComponent& transform, float spin)
    {
        Entity entity = m_Scene.CreateEntity(std::move(name));
        entity.Get<TransformComponent>() = transform;
        entity.Add<MeshRendererComponent>(MeshRendererComponent{.Mesh = &mesh, .Material = &material});
        if (spin != 0.0f)
        {
            entity.Add<SpinComponent>(SpinComponent{.Speed = spin});
        }
    }

    void SandboxLayer::SetViewportSize(std::uint32_t width, std::uint32_t height)
    {
        m_Scene.ForEach<CameraComponent>([width, height](Entity /*entity*/, CameraComponent& camera) { camera.Camera.SetViewportSize(width, height); });
    }

    void SandboxLayer::SetLooking(bool looking)
    {
        if (looking == m_Looking)
        {
            return;
        }
        m_Looking = looking;
        Application::Get().GetWindow().SetCursorCaptured(looking);

        ImGuiIO& io = ImGui::GetIO();
        constexpr ImGuiConfigFlags mouseFlags = ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange;
        io.ConfigFlags = looking ? (io.ConfigFlags | mouseFlags) : (io.ConfigFlags & ~mouseFlags);
    }
}
