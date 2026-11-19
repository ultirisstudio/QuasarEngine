#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Base.h>
#include <quasar/core/Window.h>
#include <quasar/io/EngineResources.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Camera.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Material.h>
#include <quasar/renderer/Mesh.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/Primitives.h>
#include <quasar/renderer/Renderer.h>
#include <quasar/renderer/Shader.h>
#include <quasar/scene/Entity.h>
#include <quasar/scene/Scene.h>
#include <quasar/scene/SceneRenderer.h>
#include <quasar/scene/components/CameraComponent.h>
#include <quasar/scene/components/MeshRendererComponent.h>
#include <quasar/scene/components/TransformComponent.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

using Quasar::CameraComponent;
using Quasar::Entity;
using Quasar::MeshRendererComponent;
using Quasar::TransformComponent;

namespace
{
    using Color = std::array<int, 4>;

    constexpr Color Black{0, 0, 0, 255};
    constexpr Color Red{255, 0, 0, 255};
    constexpr Color Blue{0, 0, 255, 255};

    class Fixture
    {
    public:
        Fixture() : m_Window(TestHelpers::CreateWindowWithContext())
        {
            REQUIRE(m_Window != nullptr);
            Quasar::GraphicsContext& context = *m_Window->GetGraphicsContext();
            const std::optional<Quasar::ShaderSpecification> unlit =
                Quasar::ShaderSpecification::FromFile(Quasar::GetEngineResourcesDirectory() / "shaders" / "Unlit.glsl");
            REQUIRE(unlit.has_value());
            m_Shader = context.CreateShader(*unlit);
            REQUIRE(m_Shader != nullptr);
            m_Pipeline = context.CreatePipeline({.Shader = m_Shader.get()});
            m_Framebuffer = context.CreateFramebuffer({.Width = 8, .Height = 8});
            m_Renderer = Quasar::Renderer::Create(context);
            m_Quad = Quasar::Mesh::Create(context, Quasar::Primitives::CreateQuad());
            REQUIRE(m_Pipeline != nullptr);
            REQUIRE(m_Framebuffer != nullptr);
            REQUIRE(m_Renderer != nullptr);
            REQUIRE(m_Quad != nullptr);
            m_Red = CreateColor({1.0f, 0.0f, 0.0f, 1.0f});
            m_Blue = CreateColor({0.0f, 0.0f, 1.0f, 1.0f});
            m_SceneRenderer = Quasar::CreateScope<Quasar::SceneRenderer>(*m_Renderer);
        }

        Quasar::Scene& GetScene() { return m_Scene; }
        Quasar::SceneRenderer& GetSceneRenderer() const { return *m_SceneRenderer; }
        Quasar::Framebuffer& GetFramebuffer() const { return *m_Framebuffer; }
        Quasar::Material& GetRed() const { return *m_Red; }
        Quasar::Material& GetBlue() const { return *m_Blue; }

        Entity AddQuad(Quasar::Material& material, const TransformComponent& transform = {})
        {
            Entity entity = m_Scene.CreateEntity("Quad");
            entity.Get<TransformComponent>() = transform;
            entity.Add<MeshRendererComponent>(MeshRendererComponent{.Mesh = m_Quad.get(), .Material = &material});
            return entity;
        }

        void AddQuadWithoutMaterial()
        {
            m_Scene.CreateEntity("Without material").Add<MeshRendererComponent>(MeshRendererComponent{.Mesh = m_Quad.get(), .Material = nullptr});
        }

        Entity AddCamera(const glm::vec3& position, bool primary = true)
        {
            Entity entity = m_Scene.CreateEntity("Camera");
            entity.Get<TransformComponent>().Position = position;
            auto& camera = entity.Add<CameraComponent>();
            camera.Camera = CreateCamera();
            camera.Primary = primary;
            return entity;
        }

        static Quasar::Camera CreateCamera()
        {
            Quasar::Camera camera;
            camera.SetPerspective({.VerticalFov = glm::radians(90.0f), .Near = 0.1f, .Far = 10.0f});
            return camera;
        }

        void Clear() const
        {
            m_Framebuffer->ClearColor(0, {0.0f, 0.0f, 0.0f, 1.0f});
            m_Framebuffer->ClearDepthStencil();
        }

        Color ReadColor(int x, int y) const
        {
            std::array<std::uint8_t, 4> pixel{};
            m_Framebuffer->Bind();
            glReadPixels(x, 7 - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
            m_Framebuffer->Unbind();
            return {pixel[0], pixel[1], pixel[2], pixel[3]};
        }

    private:
        Quasar::Scope<Quasar::Material> CreateColor(glm::vec4 color) const
        {
            Quasar::Scope<Quasar::Material> material = Quasar::Material::Create(
                *m_Window->GetGraphicsContext(),
                {.Pipeline = m_Pipeline.get(), .Uniforms = {{"Color", Quasar::UniformType::Vec4}}, .Textures = {{.Name = "Texture", .Slot = 0}}});
            REQUIRE(material != nullptr);
            material->SetVec4("Color", color);
            return material;
        }

        Quasar::Scope<Quasar::Window> m_Window;
        Quasar::Scope<Quasar::Shader> m_Shader;
        Quasar::Scope<Quasar::Pipeline> m_Pipeline;
        Quasar::Scope<Quasar::Framebuffer> m_Framebuffer;
        Quasar::Scope<Quasar::Renderer> m_Renderer;
        Quasar::Scope<Quasar::Mesh> m_Quad;
        Quasar::Scope<Quasar::Material> m_Red;
        Quasar::Scope<Quasar::Material> m_Blue;
        Quasar::Scope<Quasar::SceneRenderer> m_SceneRenderer;
        Quasar::Scene m_Scene;
    };

    bool IsNear(const Color& color, const Color& expected)
    {
        return std::ranges::equal(color, expected, [](int value, int other) { return std::abs(value - other) <= 1; });
    }
}

TEST_CASE("The scene renderer draws the entities that have a mesh and a material, at their transforms")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    const glm::vec3 half(0.5f);
    fixture.AddQuad(fixture.GetRed(), {.Position = {-0.5f, 0.5f, 0.0f}, .Scale = half});
    fixture.AddQuad(fixture.GetBlue(), {.Position = {0.5f, -0.5f, 0.0f}, .Scale = half});
    fixture.GetScene().CreateEntity("Empty").Add<MeshRendererComponent>();
    fixture.AddQuadWithoutMaterial();
    fixture.GetScene().CreateEntity("Without renderer");
    Quasar::Camera camera = Fixture::CreateCamera();
    camera.SetPosition({0.0f, 0.0f, 1.0f});

    fixture.Clear();
    fixture.GetSceneRenderer().Render(fixture.GetScene(), camera, &fixture.GetFramebuffer());

    CHECK(IsNear(fixture.ReadColor(1, 1), Red));
    CHECK(IsNear(fixture.ReadColor(6, 6), Blue));
    CHECK(IsNear(fixture.ReadColor(6, 1), Black));
    CHECK(IsNear(fixture.ReadColor(1, 6), Black));
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The scene renderer draws from the primary camera, at the transform of its entity")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    fixture.AddQuad(fixture.GetRed(), {.Position = {-2.0f, 0.0f, 0.0f}});
    fixture.AddQuad(fixture.GetBlue(), {.Position = {2.0f, 0.0f, 0.0f}, .Rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f))});
    fixture.AddCamera({0.0f, 0.0f, 1.0f}, false);
    const Entity camera = fixture.AddCamera({-2.0f, 0.0f, 1.0f});

    fixture.Clear();
    fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
    CHECK(IsNear(fixture.ReadColor(4, 4), Red));

    auto& transform = camera.Get<TransformComponent>();
    transform.Position = {0.0f, 0.0f, 0.0f};
    transform.Rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    transform.Scale = glm::vec3(5.0f);
    fixture.Clear();
    fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
    CHECK(IsNear(fixture.ReadColor(4, 4), Blue));
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("Without a primary camera, the scene renderer draws nothing and logs one error until it finds one again")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    fixture.AddQuad(fixture.GetRed(), {.Scale = glm::vec3(10.0f)});
    fixture.AddCamera({0.0f, 0.0f, 1.0f}, false);

    fixture.Clear();
    for (int frame = 0; frame < 3; ++frame)
    {
        fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
    }
    CHECK(IsNear(fixture.ReadColor(4, 4), Black));

    const std::vector<Quasar::LogMessage> messages = Quasar::Log::GetMemorySink()->GetMessages();
    const auto errors = std::ranges::count_if(messages, [](const Quasar::LogMessage& message) { return message.Level == Quasar::LogLevel::Error; });
    CHECK(errors == 1);

    const Entity camera = fixture.AddCamera({0.0f, 0.0f, 1.0f});
    fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
    CHECK(IsNear(fixture.ReadColor(4, 4), Red));

    camera.Get<CameraComponent>().Primary = false;
    fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
    fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
    const std::vector<Quasar::LogMessage> later = Quasar::Log::GetMemorySink()->GetMessages();
    CHECK(std::ranges::count_if(later, [](const Quasar::LogMessage& message) { return message.Level == Quasar::LogLevel::Error; }) == 2);
}
