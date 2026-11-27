#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"
#include "helpers/RenderHelpers.h"

#include <quasar/core/Base.h>
#include <quasar/core/Window.h>
#include <quasar/io/EngineResources.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Bloom.h>
#include <quasar/renderer/Camera.h>
#include <quasar/renderer/FrameProfiler.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Material.h>
#include <quasar/renderer/Mesh.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/PipelineLibrary.h>
#include <quasar/renderer/Primitives.h>
#include <quasar/renderer/RenderStatistics.h>
#include <quasar/renderer/Renderer.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/ShaderBindings.h>
#include <quasar/scene/Entity.h>
#include <quasar/scene/Scene.h>
#include <quasar/scene/SceneRenderer.h>
#include <quasar/scene/components/CameraComponent.h>
#include <quasar/scene/components/DirectionalLightComponent.h>
#include <quasar/scene/components/MeshRendererComponent.h>
#include <quasar/scene/components/PointLightComponent.h>
#include <quasar/scene/components/SpotLightComponent.h>
#include <quasar/scene/components/TransformComponent.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using Quasar::CameraComponent;
using Quasar::DirectionalLightComponent;
using Quasar::Entity;
using Quasar::MeshRendererComponent;
using Quasar::PointLightComponent;
using Quasar::SpotLightComponent;
using Quasar::TransformComponent;
using TestHelpers::Color;
using TestHelpers::IsNear;
using TestHelpers::ReadWindowPixel;

namespace
{
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
            m_SceneRenderer = Quasar::SceneRenderer::Create(*m_Renderer, {.Background = glm::vec3(0.0f), .Tonemap = Quasar::Tonemapping::None});
            REQUIRE(m_SceneRenderer != nullptr);
        }

        Quasar::GraphicsContext& GetContext() const { return *m_Window->GetGraphicsContext(); }
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

        Color ReadColor(int x, int y) const { return TestHelpers::ReadColor(*m_Framebuffer, x, y); }

        Quasar::Scope<Quasar::Material> CreateColor(glm::vec4 color) const
        {
            Quasar::Scope<Quasar::Material> material = Quasar::Material::Create(
                *m_Window->GetGraphicsContext(),
                {.Pipeline = m_Pipeline.get(), .Uniforms = {{"Color", Quasar::UniformType::Vec4}}, .Textures = {{.Name = "Texture", .Slot = 0}}});
            REQUIRE(material != nullptr);
            material->SetVec4("Color", color);
            return material;
        }

    private:
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

TEST_CASE("Once its materials are sent, the scene renderer draws a frame without uploads to the driver")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    fixture.AddQuad(fixture.GetRed(), {.Position = {-0.5f, 0.5f, 0.0f}, .Scale = glm::vec3(0.5f)});
    fixture.AddQuad(fixture.GetBlue(), {.Position = {0.5f, -0.5f, 0.0f}, .Scale = glm::vec3(0.5f)});
    Entity sun = fixture.GetScene().CreateEntity("Sun");
    sun.Add<DirectionalLightComponent>();
    Quasar::Camera camera = Fixture::CreateCamera();
    camera.SetPosition({0.0f, 0.0f, 1.0f});
    Quasar::GraphicsContext& context = fixture.GetContext();

    for (int frame = 0; frame < 2; ++frame)
    {
        fixture.Clear();
        fixture.GetSceneRenderer().Render(fixture.GetScene(), camera, &fixture.GetFramebuffer());
        context.EndFrame();
    }

    const Quasar::RenderStatistics& statistics = context.GetFrameStatistics();
    CHECK(statistics.Draws > 4);
    CHECK(statistics.Uploads == 0);
    CHECK(statistics.UploadedBytes > 0);
    CHECK(IsNear(fixture.ReadColor(1, 1), Red));
    CHECK(IsNear(fixture.ReadColor(6, 6), Blue));
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("The meshes out of the view of the camera are not drawn, and the ones out of the view of the light cast no shadow")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    fixture.AddQuad(fixture.GetRed()).Get<MeshRendererComponent>().CastShadows = false;
    const std::vector<Entity> hidden{fixture.AddQuad(fixture.GetBlue(), {.Position = {0.0f, 0.0f, 3.0f}}),
                                     fixture.AddQuad(fixture.GetBlue(), {.Position = {20.0f, 0.0f, 0.0f}}),
                                     fixture.AddQuad(fixture.GetBlue(), {.Position = {0.0f, 0.0f, -20.0f}})};
    Quasar::Camera camera = Fixture::CreateCamera();
    camera.SetPosition({0.0f, 0.0f, 1.0f});

    fixture.Clear();
    fixture.GetSceneRenderer().Render(fixture.GetScene(), camera, &fixture.GetFramebuffer());
    CHECK(fixture.GetSceneRenderer().GetStatistics() == Quasar::SceneRendererStatistics{.Objects = 4, .VisibleObjects = 1, .ShadowCasters = 0});
    CHECK(IsNear(fixture.ReadColor(3, 3), Red));

    Entity sun = fixture.GetScene().CreateEntity("Sun");
    sun.Add<DirectionalLightComponent>();
    sun.Get<TransformComponent>().Rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    fixture.GetSceneRenderer().Render(fixture.GetScene(), camera, &fixture.GetFramebuffer());
    CHECK(fixture.GetSceneRenderer().GetStatistics() == Quasar::SceneRendererStatistics{.Objects = 4, .VisibleObjects = 1, .ShadowCasters = 1});

    fixture.GetSceneRenderer().SetSettings({.Shadows = {.Enabled = false}});
    fixture.GetSceneRenderer().Render(fixture.GetScene(), camera, &fixture.GetFramebuffer());
    CHECK(fixture.GetSceneRenderer().GetStatistics() == Quasar::SceneRendererStatistics{.Objects = 4, .VisibleObjects = 1, .ShadowCasters = 0});

    Quasar::GraphicsContext& context = fixture.GetContext();
    context.EndFrame();
    fixture.GetSceneRenderer().Render(fixture.GetScene(), camera, &fixture.GetFramebuffer());
    context.EndFrame();
    const std::uint64_t trianglesWithoutShadows = context.GetFrameStatistics().Triangles;
    fixture.GetSceneRenderer().SetSettings({});
    fixture.GetSceneRenderer().Render(fixture.GetScene(), camera, &fixture.GetFramebuffer());
    context.EndFrame();
    const std::uint64_t culledTriangles = context.GetFrameStatistics().Triangles;
    const std::uint64_t quad = 2;
    CHECK(culledTriangles == trianglesWithoutShadows + quad);
    for (const Entity& entity : hidden)
    {
        entity.Get<TransformComponent>().Position = {0.0f, 0.0f, -1.0f};
    }
    fixture.GetSceneRenderer().Render(fixture.GetScene(), camera, &fixture.GetFramebuffer());
    context.EndFrame();
    CHECK(context.GetFrameStatistics().Triangles == culledTriangles + (8 * quad));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The scene renderer draws the meshes that share a mesh and a material in one draw, whatever the order of the entities")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    constexpr int count = 8;
    std::vector<Entity> blue;
    for (int index = 0; index < count; ++index)
    {
        const float x = -0.875f + (0.25f * static_cast<float>(index));
        const Entity entity = fixture.AddQuad(index % 2 == 0 ? fixture.GetRed() : fixture.GetBlue(), {.Position = {x, 0.0f, 0.0f}, .Scale = glm::vec3(0.25f)});
        if (index % 2 != 0)
        {
            blue.push_back(entity);
        }
    }
    Quasar::Camera camera = Fixture::CreateCamera();
    camera.SetPosition({0.0f, 0.0f, 1.0f});
    fixture.GetSceneRenderer().SetSettings({.Tonemap = Quasar::Tonemapping::None, .Shadows = {.Enabled = false}, .AmbientOcclusion = {.Enabled = false}});
    Quasar::GraphicsContext& context = fixture.GetContext();
    const auto render = [&]
    {
        fixture.Clear();
        fixture.GetSceneRenderer().Render(fixture.GetScene(), camera, &fixture.GetFramebuffer());
        context.EndFrame();
        return context.GetFrameStatistics();
    };

    render();
    const Quasar::RenderStatistics twoMaterials = render();
    const std::uint32_t cameraAndLights = 2;
    const std::uint32_t materials = 2;
    CHECK(twoMaterials.UniformBufferBinds == cameraAndLights + materials);
    CHECK(twoMaterials.StorageBufferBinds == 1);
    for (int pixel = 0; pixel < 8; ++pixel)
    {
        CAPTURE(pixel);
        const Color color = fixture.ReadColor(pixel, 4);
        CHECK((pixel % 2 == 0 ? color[0] > color[2] + 100 : color[2] > color[0] + 100));
    }

    for (const Entity& entity : blue)
    {
        entity.Get<MeshRendererComponent>().Material = &fixture.GetRed();
    }
    const Quasar::RenderStatistics oneMaterial = render();
    CHECK(twoMaterials.Draws == oneMaterial.Draws + 1);
    CHECK(twoMaterials.Triangles == oneMaterial.Triangles);
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("The scene renderer places each mesh once for the image and the shadow map, and draws its casters in one draw")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    constexpr int count = 8;
    const auto addQuads = [&fixture](float y)
    {
        for (int index = 0; index < count; ++index)
        {
            const float x = -0.875f + (0.25f * static_cast<float>(index));
            Entity quad = fixture.AddQuad(fixture.GetRed(), {.Position = {x, y, 0.0f}, .Scale = glm::vec3(0.25f)});
            quad.Get<MeshRendererComponent>().CastShadows = index % 2 == 0;
        }
    };
    addQuads(0.0f);
    Entity sun = fixture.GetScene().CreateEntity("Sun");
    sun.Add<DirectionalLightComponent>();
    sun.Get<TransformComponent>().Rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    Quasar::Camera camera = Fixture::CreateCamera();
    camera.SetPosition({0.0f, 0.0f, 1.0f});
    Quasar::GraphicsContext& context = fixture.GetContext();
    const auto render = [&](bool shadows)
    {
        fixture.GetSceneRenderer().SetSettings({.Shadows = {.Enabled = shadows}, .AmbientOcclusion = {.Enabled = false}});
        fixture.GetSceneRenderer().Render(fixture.GetScene(), camera, &fixture.GetFramebuffer());
        context.EndFrame();
        return context.GetFrameStatistics();
    };

    render(true);
    const Quasar::RenderStatistics withShadows = render(true);
    CHECK(withShadows.Draws == render(false).Draws + 1);

    addQuads(0.5f);
    const Quasar::RenderStatistics twiceAsMany = render(true);
    CHECK(fixture.GetSceneRenderer().GetStatistics() ==
          Quasar::SceneRendererStatistics{.Objects = 2 * count, .VisibleObjects = 2 * count, .ShadowCasters = count});
    const std::uint64_t objectBytes = 112;
    CHECK(twiceAsMany.UploadedBytes == withShadows.UploadedBytes + (count * objectBytes));
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

TEST_CASE("The scene renderer lights the scene with its directional light, and without one the scene is dark")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    Quasar::PipelineLibrary library(fixture.GetContext());
    REQUIRE(library.AddDefaults());
    const Quasar::Scope<Quasar::Material> lit = library.CreateMaterial(Quasar::PipelineLibrary::BlinnPhong);
    REQUIRE(lit != nullptr);
    lit->SetFloat("Specular", 0.0f);
    fixture.AddQuad(*lit);
    fixture.AddCamera({0.0f, 0.0f, 1.0f});
    const auto render = [&fixture]
    {
        fixture.Clear();
        fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
        return fixture.ReadColor(4, 4);
    };

    CHECK(IsNear(render(), {0, 0, 0, 255}));

    Entity sun = fixture.GetScene().CreateEntity("Sun");
    auto& component = sun.Add<DirectionalLightComponent>(DirectionalLightComponent{.Ambient = 0.0f});
    CHECK(IsNear(render(), {255, 255, 255, 255}));

    sun.Get<TransformComponent>().Rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    component = {.Color = {1.0f, 0.0f, 0.0f}, .Intensity = 0.5f, .Ambient = 1.0f};
    CHECK(IsNear(render(), {188, 0, 0, 255}));

    const Entity rig = fixture.GetScene().CreateEntity("Rig");
    rig.Get<TransformComponent>().Rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    REQUIRE(sun.SetParent(rig, false));
    component = {.Ambient = 0.0f};
    CHECK(IsNear(render(), {255, 255, 255, 255}));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The scene renderer places the meshes and the camera at their places in the world, under their parents")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    const Entity holder = fixture.GetScene().CreateEntity("Holder");
    holder.Get<TransformComponent>().Position = {-2.0f, 0.0f, 0.0f};
    Entity quad = fixture.AddQuad(fixture.GetRed(), {.Position = {2.0f, 0.0f, 0.0f}});
    REQUIRE(quad.SetParent(holder, false));

    const Entity rig = fixture.GetScene().CreateEntity("Rig");
    rig.Get<TransformComponent>().Rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    Entity camera = fixture.AddCamera({0.0f, 0.0f, -1.0f});
    camera.Get<TransformComponent>().Rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    REQUIRE(camera.SetParent(rig, false));

    fixture.Clear();
    fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
    CHECK(IsNear(fixture.ReadColor(4, 4), Red));

    holder.Get<TransformComponent>().Position = {-10.0f, 0.0f, 0.0f};
    fixture.Clear();
    fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
    CHECK(IsNear(fixture.ReadColor(4, 4), Black));
}

TEST_CASE("The scene renderer brings the linear colors of the scene back to sRGB, over its background")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    const Quasar::Scope<Quasar::Material> gray = fixture.CreateColor({0.5f, 0.5f, 0.5f, 1.0f});
    fixture.AddQuad(*gray);
    fixture.AddCamera({0.0f, 0.0f, 1.0f});
    fixture.GetSceneRenderer().SetSettings({.Background = {0.25f, 0.5f, 1.0f}, .Tonemap = Quasar::Tonemapping::None});

    fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());

    CHECK(IsNear(fixture.ReadColor(4, 4), {128, 128, 128, 255}));
    CHECK(IsNear(fixture.ReadColor(0, 0), {64, 128, 255, 255}));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The exposure and the tonemapping bring the colors above 1 back to the screen")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    const Quasar::Scope<Quasar::Material> white = fixture.CreateColor(glm::vec4(1.0f));
    fixture.AddQuad(*white);
    fixture.AddCamera({0.0f, 0.0f, 1.0f});
    const auto render = [&fixture](float exposure, Quasar::Tonemapping tonemap)
    {
        fixture.GetSceneRenderer().SetSettings({.Exposure = exposure, .Tonemap = tonemap});
        fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
        return fixture.ReadColor(4, 4)[0];
    };

    CHECK(render(2.0f, Quasar::Tonemapping::None) == 255);
    CHECK(std::abs(render(0.5f, Quasar::Tonemapping::None) - 188) <= 1);
    CHECK(std::abs(render(1.0f, Quasar::Tonemapping::Reinhard) - 188) <= 1);
    CHECK(std::abs(render(2.0f, Quasar::Tonemapping::Reinhard) - 213) <= 1);
    CHECK(std::abs(render(1.0f, Quasar::Tonemapping::Aces) - 232) <= 1);
    CHECK(std::abs(render(2.0f, Quasar::Tonemapping::Aces) - 245) <= 1);
    CHECK(fixture.GetSceneRenderer().GetSettings().Exposure == 2.0f);
    CHECK(fixture.GetSceneRenderer().GetSettings().Tonemap == Quasar::Tonemapping::Aces);
}

TEST_CASE("The scene framebuffer takes the size of the target, or of the window, and the window receives the image")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    fixture.AddCamera({0.0f, 0.0f, 1.0f});
    fixture.GetSceneRenderer().SetSettings({.Background = {0.0f, 1.0f, 0.0f}, .Tonemap = Quasar::Tonemapping::None});
    const auto scene = [&fixture]() -> const Quasar::FramebufferSpecification& { return fixture.GetSceneRenderer().GetSceneFramebuffer().GetSpecification(); };
    CHECK(scene().Attachments.front() == Quasar::FramebufferFormat::Rgba16F);

    fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
    CHECK(scene().Width == 8);
    CHECK(scene().Height == 8);

    const Quasar::Scope<Quasar::Framebuffer> wide = fixture.GetContext().CreateFramebuffer({.Width = 16, .Height = 4});
    REQUIRE(wide != nullptr);
    fixture.GetSceneRenderer().Render(fixture.GetScene(), wide.get());
    CHECK(scene().Width == 16);
    CHECK(scene().Height == 4);

    fixture.GetContext().ClearWindow({1.0f, 0.0f, 0.0f, 1.0f});
    fixture.GetSceneRenderer().Render(fixture.GetScene());
    const glm::uvec2 window = fixture.GetContext().GetWindowSize();
    CHECK(scene().Width == window.x);
    CHECK(scene().Height == window.y);
    CHECK(IsNear(ReadWindowPixel(static_cast<int>(window.x) - 1, static_cast<int>(window.y) - 1), {0, 255, 0, 255}));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The scene renderer lights the scene with its point and spot lights, at their places in the world")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    Quasar::PipelineLibrary library(fixture.GetContext());
    REQUIRE(library.AddDefaults());
    const Quasar::Scope<Quasar::Material> lit = library.CreateMaterial(Quasar::PipelineLibrary::BlinnPhong);
    REQUIRE(lit != nullptr);
    lit->SetFloat("Specular", 0.0f);
    fixture.AddQuad(*lit);
    fixture.AddCamera({0.0f, 0.0f, 1.0f});
    const auto render = [&fixture]
    {
        fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
        return fixture.ReadColor(4, 4);
    };
    constexpr Color fullyLit{250, 250, 250, 255};

    const Entity holder = fixture.GetScene().CreateEntity("Holder");
    Entity lamp = fixture.GetScene().CreateEntity("Lamp");
    lamp.Get<TransformComponent>().Position = {0.0f, 0.0f, 1.0f};
    auto& bulb = lamp.Add<PointLightComponent>(PointLightComponent{.Intensity = 0.5f});
    REQUIRE(lamp.SetParent(holder, false));
    CHECK(IsNear(render(), {184, 184, 184, 255}));
    bulb.Intensity = 1.0f;
    CHECK(IsNear(render(), fullyLit));
    holder.Get<TransformComponent>().Position = {0.0f, 0.0f, 20.0f};
    CHECK(IsNear(render(), Black));

    const Entity rig = fixture.GetScene().CreateEntity("Rig");
    rig.Get<TransformComponent>().Rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    Entity torch = fixture.GetScene().CreateEntity("Torch");
    torch.Get<TransformComponent>() = {.Position = {0.0f, 0.0f, -1.0f}, .Rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f))};
    torch.Add<SpotLightComponent>();
    REQUIRE(torch.SetParent(rig, false));
    CHECK(IsNear(render(), fullyLit));
    rig.Get<TransformComponent>().Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    CHECK(IsNear(render(), Black));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

namespace
{
    struct ShadowScene
    {
        explicit ShadowScene(Fixture& fixture) : Library(fixture.GetContext())
        {
            REQUIRE(Library.AddDefaults());
            Lit = Library.CreateMaterial(Quasar::PipelineLibrary::BlinnPhong);
            REQUIRE(Lit != nullptr);
            Lit->SetFloat("Specular", 0.0f);
            const glm::quat facingUp = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            fixture.AddQuad(*Lit, {.Rotation = facingUp, .Scale = glm::vec3(20.0f)});
            Occluder = fixture.AddQuad(*Lit, {.Position = {0.0f, 3.0f, 0.0f}, .Rotation = facingUp, .Scale = glm::vec3(4.0f)});
            Camera = fixture.AddCamera({0.0f, 1.0f, 0.0f});
            Camera.Get<TransformComponent>().Rotation = facingUp;
            Sun = fixture.GetScene().CreateEntity("Sun");
            Sun.Get<TransformComponent>().Rotation = facingUp;
            Sun.Add<DirectionalLightComponent>(DirectionalLightComponent{.Ambient = 0.1f});
        }

        Quasar::PipelineLibrary Library;
        Quasar::Scope<Quasar::Material> Lit;
        Entity Occluder;
        Entity Camera;
        Entity Sun;
    };

    constexpr Color Shadowed{89, 89, 89, 255};
    constexpr Color White{255, 255, 255, 255};
}

TEST_CASE("The meshes hide the directional light from the surfaces behind them, even out of the view of the camera")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    const ShadowScene scene(fixture);
    const auto render = [&fixture](int x, int y)
    {
        fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
        return fixture.ReadColor(x, y);
    };

    CHECK(IsNear(render(4, 4), Shadowed, 3));
    CHECK(IsNear(render(1, 6), Shadowed, 3));
    CHECK(fixture.GetSceneRenderer().GetShadowMap().GetSpecification().Width == 2048);

    scene.Occluder.Get<TransformComponent>().Position.x = 30.0f;
    CHECK(IsNear(render(4, 4), White));
    scene.Occluder.Get<TransformComponent>().Position.x = 0.0f;

    scene.Occluder.Get<MeshRendererComponent>().CastShadows = false;
    CHECK(IsNear(render(4, 4), White));
    scene.Occluder.Get<MeshRendererComponent>().CastShadows = true;
    CHECK(IsNear(render(4, 4), Shadowed, 3));

    scene.Sun.Get<DirectionalLightComponent>().CastShadows = false;
    CHECK(IsNear(render(4, 4), White));
    scene.Sun.Get<DirectionalLightComponent>().CastShadows = true;
    CHECK(IsNear(render(4, 4), Shadowed, 3));

    Quasar::SceneRendererSettings settings = fixture.GetSceneRenderer().GetSettings();
    settings.Shadows.Enabled = false;
    fixture.GetSceneRenderer().SetSettings(settings);
    const Quasar::Scope<Quasar::Framebuffer> closest =
        fixture.GetContext().CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {Quasar::FramebufferFormat::Depth32F}});
    REQUIRE(closest != nullptr);
    closest->ClearDepthStencil(0.0f);
    closest->BindDepthTexture(Quasar::ShaderBindings::ShadowMap);
    CHECK(IsNear(render(4, 4), White));

    settings.Shadows = {.Resolution = 512};
    fixture.GetSceneRenderer().SetSettings(settings);
    CHECK(IsNear(render(4, 4), Shadowed, 3));
    CHECK(fixture.GetSceneRenderer().GetShadowMap().GetSpecification().Width == 512);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A surface lit at an angle does not shadow itself, and an orthographic camera also gets shadows")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    const ShadowScene scene(fixture);
    scene.Occluder.Get<TransformComponent>().Position.x = 30.0f;
    scene.Sun.Get<TransformComponent>().Rotation =
        glm::angleAxis(glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());

    for (const auto& [x, y] : {std::pair{1, 1}, std::pair{4, 4}, std::pair{6, 6}, std::pair{1, 6}, std::pair{6, 1}})
    {
        CAPTURE(x);
        CAPTURE(y);
        CHECK(IsNear(fixture.ReadColor(x, y), {232, 232, 232, 255}));
    }

    scene.Occluder.Get<TransformComponent>().Position.x = 0.0f;
    scene.Sun.Get<TransformComponent>().Rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    scene.Camera.Get<CameraComponent>().Camera.SetOrthographic({.Height = 2.0f, .Near = 0.1f, .Far = 10.0f});
    fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
    CHECK(IsNear(fixture.ReadColor(4, 4), Shadowed, 3));
}

TEST_CASE("The shadow map moves by whole pixels when the camera moves or turns, so that the shadows do not shimmer")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    const ShadowScene scene(fixture);
    scene.Sun.Get<TransformComponent>().Rotation =
        glm::angleAxis(glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::angleAxis(glm::radians(-60.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    const auto pixelOfOrigin = [&fixture]
    {
        fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
        const glm::vec4 clip = fixture.GetSceneRenderer().GetShadowViewProjection() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        return ((glm::vec2(clip) / clip.w) * 0.5f + 0.5f) * 2048.0f;
    };
    const glm::vec2 first = pixelOfOrigin();

    for (const glm::vec3& move : {glm::vec3(0.0137f, 0.0f, 0.0f), glm::vec3(0.0f, 0.021f, 0.0091f), glm::vec3(1.37f, 0.4f, -2.2f)})
    {
        CAPTURE(move.x);
        scene.Camera.Get<TransformComponent>().Position += move;
        scene.Camera.Get<TransformComponent>().Rotation = glm::angleAxis(0.013f, glm::vec3(0.0f, 1.0f, 0.0f)) * scene.Camera.Get<TransformComponent>().Rotation;
        const glm::vec2 moved = pixelOfOrigin();
        const glm::vec2 shift = moved - first;
        CHECK(std::abs(shift.x - std::round(shift.x)) < 0.01f);
        CHECK(std::abs(shift.y - std::round(shift.y)) < 0.01f);
    }
}

TEST_CASE("A surface lit at a grazing angle does not shadow itself, thanks to the biases")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    const ShadowScene scene(fixture);
    scene.Occluder.Get<TransformComponent>().Position.x = 30.0f;
    scene.Sun.Get<TransformComponent>().Rotation =
        glm::angleAxis(glm::radians(80.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    fixture.GetSceneRenderer().SetSettings({.Tonemap = Quasar::Tonemapping::None, .Shadows = {.Resolution = 128}});
    const auto lowest = [&fixture]
    {
        fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
        int value = 255;
        for (int y = 0; y < 8; ++y)
        {
            for (int x = 0; x < 8; ++x)
            {
                value = std::min(value, fixture.ReadColor(x, y)[0]);
            }
        }
        return value;
    };

    CHECK(lowest() > 130);

    fixture.GetSceneRenderer().SetSettings({.Tonemap = Quasar::Tonemapping::None, .Shadows = {.Resolution = 128, .DepthBias = 0.0f, .NormalBias = 0.0f}});
    CHECK(lowest() < 125);

    fixture.GetSceneRenderer().SetSettings({.Tonemap = Quasar::Tonemapping::None, .Shadows = {.Resolution = 128, .DepthBias = 0.03f, .NormalBias = 0.0f}});
    CHECK(lowest() > 130);
}

TEST_CASE("The shadow map covers the view only up to the shadow distance, and the surfaces beyond it are lit")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    const ShadowScene scene(fixture);
    fixture.GetSceneRenderer().SetSettings({.Tonemap = Quasar::Tonemapping::None, .Shadows = {.Distance = 3.0f}});
    const auto isOnMap = [&fixture](const glm::vec3& point)
    {
        fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
        const glm::vec4 clip = fixture.GetSceneRenderer().GetShadowViewProjection() * glm::vec4(point, 1.0f);
        return std::abs(clip.x / clip.w) <= 1.0f && std::abs(clip.y / clip.w) <= 1.0f;
    };

    CHECK(isOnMap({0.0f, 0.0f, 0.0f}));
    CHECK_FALSE(isOnMap({10.0f, 0.0f, 0.0f}));
    scene.Camera.Get<CameraComponent>().Camera.SetOrthographic({.Height = 2.0f, .Near = 0.1f, .Far = 1000.0f});
    CHECK(isOnMap({0.0f, 0.0f, 0.0f}));
    CHECK_FALSE(isOnMap({10.0f, 0.0f, 0.0f}));

    scene.Camera.Get<CameraComponent>().Camera.SetPerspective({.VerticalFov = glm::radians(90.0f), .Near = 0.1f, .Far = 10.0f});
    scene.Occluder.Get<TransformComponent>().Position.x = 30.0f;
    fixture.GetScene().ForEach<MeshRendererComponent, TransformComponent>(
        [](Entity /*entity*/, const MeshRendererComponent& /*renderer*/, TransformComponent& transform)
        {
            if (transform.Scale.x == 20.0f)
            {
                transform.Position.y = -6.0f;
            }
        });
    fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
    CHECK(IsNear(fixture.ReadColor(4, 4), White));
}

TEST_CASE("The shadows have soft edges, over a few pixels of the shadow map")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    const ShadowScene scene(fixture);
    scene.Camera.Get<TransformComponent>().Position.x = 2.0f;
    fixture.GetSceneRenderer().SetSettings({.Tonemap = Quasar::Tonemapping::None, .Shadows = {.Resolution = 32, .Distance = 3.0f}});
    const Quasar::Scope<Quasar::Framebuffer> wide = fixture.GetContext().CreateFramebuffer({.Width = 64, .Height = 64});
    REQUIRE(wide != nullptr);

    fixture.GetSceneRenderer().Render(fixture.GetScene(), wide.get());
    std::vector<std::uint8_t> row(std::size_t{64} * 4);
    wide->Bind();
    glReadPixels(0, 32, 64, 1, GL_RGBA, GL_UNSIGNED_BYTE, row.data());
    wide->Unbind();

    int between = 0;
    for (std::size_t x = 0; x < 64; ++x)
    {
        between += row[x * 4] > Shadowed[0] + 10 && row[x * 4] < 245 ? 1 : 0;
    }
    CAPTURE(between);
    CHECK(between >= 10);
    CHECK(row[0] < Shadowed[0] + 10);
    CHECK(row[std::size_t{63} * 4] > 245);
}

TEST_CASE("Several samples per pixel smooth the edges of the meshes, up to the limit of the GPU")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    const Quasar::Scope<Quasar::Material> white = fixture.CreateColor(glm::vec4(1.0f));
    fixture.AddQuad(*white, {.Rotation = glm::angleAxis(glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f)), .Scale = glm::vec3(1.2f)});
    fixture.AddCamera({0.0f, 0.0f, 1.0f});
    const auto partlyCovered = [&fixture](std::uint32_t samples)
    {
        fixture.GetSceneRenderer().SetSettings({.Background = glm::vec3(0.0f), .Tonemap = Quasar::Tonemapping::None, .Samples = samples});
        fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
        int count = 0;
        for (int y = 0; y < 8; ++y)
        {
            for (int x = 0; x < 8; ++x)
            {
                const int red = fixture.ReadColor(x, y)[0];
                count += red > 10 && red < 245 ? 1 : 0;
            }
        }
        return count;
    };
    const std::uint32_t limit = fixture.GetContext().GetMaxSamples();

    CHECK(partlyCovered(1) == 0);
    CHECK(fixture.GetSceneRenderer().GetSceneFramebuffer().GetSpecification().Samples == 1);
    CHECK(partlyCovered(0) == 0);
    CHECK(fixture.GetSceneRenderer().GetSceneFramebuffer().GetSpecification().Samples == 1);
    if (limit >= 4)
    {
        CHECK(partlyCovered(4) > 4);
        CHECK(fixture.GetSceneRenderer().GetSceneFramebuffer().GetSpecification().Samples == 4);
    }
    CHECK(partlyCovered(1000) > 0);
    CHECK(fixture.GetSceneRenderer().GetSceneFramebuffer().GetSpecification().Samples == limit);
    CHECK(fixture.GetSceneRenderer().GetSettings().Samples == 1000);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The ambient occlusion darkens the ambient light where surfaces meet, and the direct lights only when asked")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    Quasar::PipelineLibrary library(fixture.GetContext());
    REQUIRE(library.AddDefaults());
    const Quasar::Scope<Quasar::Material> lit = library.CreateMaterial(Quasar::PipelineLibrary::BlinnPhong);
    REQUIRE(lit != nullptr);
    lit->SetFloat("Specular", 0.0f);
    const glm::quat facingUp = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    fixture.AddQuad(*lit, {.Rotation = facingUp, .Scale = glm::vec3(20.0f)});
    fixture.AddQuad(*lit, {.Position = {0.0f, 1.0f, 0.0f}, .Scale = glm::vec3(2.0f)});
    Entity camera = fixture.AddCamera({0.0f, 1.5f, 2.5f});
    camera.Get<CameraComponent>().Camera.SetViewportSize(64, 64);
    camera.Get<TransformComponent>().Rotation =
        glm::quatLookAtRH(glm::normalize(glm::vec3(0.0f, 0.3f, 0.0f) - glm::vec3(0.0f, 1.5f, 2.5f)), {0.0f, 1.0f, 0.0f});
    Entity sun = fixture.GetScene().CreateEntity("Sun");
    auto& light = sun.Add<DirectionalLightComponent>(DirectionalLightComponent{.Ambient = 1.0f});
    sun.Get<TransformComponent>().Rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    const Quasar::Scope<Quasar::Framebuffer> target = fixture.GetContext().CreateFramebuffer({.Width = 64, .Height = 64});
    REQUIRE(target != nullptr);
    const Quasar::Camera& view = camera.Get<CameraComponent>().Camera;
    const auto render = [&](const Quasar::AmbientOcclusionSettings& occlusion, const glm::vec3& point)
    {
        fixture.GetSceneRenderer().SetSettings({.Tonemap = Quasar::Tonemapping::None, .Shadows = {.Enabled = false}, .AmbientOcclusion = occlusion});
        fixture.GetSceneRenderer().Render(fixture.GetScene(), target.get());
        Quasar::Camera placed = view;
        placed.SetPosition(camera.Get<TransformComponent>().Position);
        placed.SetOrientation(camera.Get<TransformComponent>().Rotation);
        const glm::vec4 clip = placed.GetViewProjection() * glm::vec4(point, 1.0f);
        const glm::vec2 pixel = (glm::vec2(clip) / clip.w * 0.5f + 0.5f) * 64.0f;
        return TestHelpers::ReadColor(*target, static_cast<int>(pixel.x), 63 - static_cast<int>(pixel.y))[0];
    };
    const glm::vec3 crease(0.0f, 0.0f, 0.08f);
    const glm::vec3 open(0.0f, 0.0f, 1.6f);

    const int ambient = render({.Enabled = false}, open);
    CHECK(ambient > 200);
    CHECK(std::abs(render({.Enabled = false}, crease) - ambient) <= 2);
    CHECK(render({.HalfResolution = false}, crease) < ambient - 20);
    CHECK(std::abs(render({.HalfResolution = false}, open) - ambient) <= 3);
    CHECK(render({}, crease) < ambient - 15);
    CHECK(std::abs(render({}, open) - ambient) <= 3);
    CHECK(std::abs(render({.Enabled = false}, crease) - ambient) <= 2);

    light.Ambient = 0.0f;
    sun.Get<TransformComponent>().Rotation = facingUp;
    const int direct = render({.Enabled = false}, crease);
    CHECK(direct > 200);
    CHECK(std::abs(render({.DirectLightingStrength = 0.0f}, crease) - direct) <= 2);
    CHECK(render({.HalfResolution = false, .DirectLightingStrength = 1.0f}, crease) < direct - 20);
    CHECK(render({.DirectLightingStrength = 3.0f}, crease) == render({.DirectLightingStrength = 1.0f}, crease));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The ambient occlusion sees the sides of the meshes that their materials draw, and only them")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    Quasar::PipelineLibrary library(fixture.GetContext());
    REQUIRE(library.AddDefaults());
    const Quasar::Scope<Quasar::Material> lit = library.CreateMaterial(Quasar::PipelineLibrary::BlinnPhong);
    REQUIRE(lit != nullptr);
    lit->SetFloat("Specular", 0.0f);
    const Quasar::Scope<Quasar::Pipeline> twoSidedPipeline =
        fixture.GetContext().CreatePipeline({.Shader = lit->GetSpecification().Pipeline->GetSpecification().Shader, .Culling = Quasar::CullMode::None});
    REQUIRE(twoSidedPipeline != nullptr);
    Quasar::MaterialSpecification twoSidedSpecification = lit->GetSpecification();
    twoSidedSpecification.Pipeline = twoSidedPipeline.get();
    const Quasar::Scope<Quasar::Material> twoSided = Quasar::Material::Create(fixture.GetContext(), twoSidedSpecification);
    REQUIRE(twoSided != nullptr);
    twoSided->SetFloat("Specular", 0.0f);

    const glm::quat facingUp = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    fixture.AddQuad(*lit, {.Rotation = facingUp, .Scale = glm::vec3(20.0f)});
    const Entity wall = fixture.AddQuad(
        *lit, {.Position = {0.0f, 1.0f, 0.0f}, .Rotation = glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)), .Scale = glm::vec3(2.0f)});
    Entity camera = fixture.AddCamera({0.0f, 1.5f, 2.5f});
    camera.Get<CameraComponent>().Camera.SetViewportSize(64, 64);
    camera.Get<TransformComponent>().Rotation =
        glm::quatLookAtRH(glm::normalize(glm::vec3(0.0f, 0.3f, 0.0f) - glm::vec3(0.0f, 1.5f, 2.5f)), {0.0f, 1.0f, 0.0f});
    Entity sun = fixture.GetScene().CreateEntity("Sun");
    sun.Add<DirectionalLightComponent>(DirectionalLightComponent{.Ambient = 1.0f});
    sun.Get<TransformComponent>().Rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    const Quasar::Scope<Quasar::Framebuffer> target = fixture.GetContext().CreateFramebuffer({.Width = 64, .Height = 64});
    REQUIRE(target != nullptr);
    const Quasar::Camera& view = camera.Get<CameraComponent>().Camera;
    const auto render = [&](bool occlusion)
    {
        fixture.GetSceneRenderer().SetSettings(
            {.Tonemap = Quasar::Tonemapping::None, .Shadows = {.Enabled = false}, .AmbientOcclusion = {.Enabled = occlusion, .HalfResolution = false}});
        fixture.GetSceneRenderer().Render(fixture.GetScene(), target.get());
        Quasar::Camera placed = view;
        placed.SetPosition(camera.Get<TransformComponent>().Position);
        placed.SetOrientation(camera.Get<TransformComponent>().Rotation);
        const glm::vec4 clip = placed.GetViewProjection() * glm::vec4(0.0f, 0.0f, 0.08f, 1.0f);
        const glm::vec2 pixel = (glm::vec2(clip) / clip.w * 0.5f + 0.5f) * 64.0f;
        return TestHelpers::ReadColor(*target, static_cast<int>(pixel.x), 63 - static_cast<int>(pixel.y))[0];
    };

    const int ambient = render(false);
    CHECK(ambient > 200);
    CHECK(std::abs(render(true) - ambient) <= 3);

    wall.Get<MeshRendererComponent>().Material = twoSided.get();
    CHECK(render(true) < ambient - 20);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The scene renderer measures each of its steps, from the culling to the post-processing")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    const ShadowScene scene(fixture);
    Quasar::FrameProfiler& profiler = fixture.GetContext().GetProfiler();
    profiler.SetEnabled(true);
    REQUIRE(fixture.GetSceneRenderer().GetPostProcessing().Add(Quasar::Bloom::Create(fixture.GetContext())) != nullptr);

    for (std::size_t frame = 0; frame <= Quasar::FrameProfiler::FramesInFlight && profiler.GetTimings().empty(); ++frame)
    {
        profiler.BeginFrame();
        fixture.GetSceneRenderer().Render(fixture.GetScene(), &fixture.GetFramebuffer());
        profiler.EndFrame();
        glFinish();
    }

    std::vector<std::pair<std::string, std::uint32_t>> sections;
    for (const Quasar::SectionTiming& timing : profiler.GetTimings())
    {
        sections.emplace_back(timing.Name, timing.Depth);
        CHECK(timing.GpuMilliseconds >= 0.0);
    }
    const std::vector<std::pair<std::string, std::uint32_t>> expected{
        {"Gather and cull", 0}, {"Sort and batch", 0}, {"Shadows", 0},         {"Depth prepass", 0}, {"Ambient occlusion", 0}, {"Occlusion", 1}, {"Blur", 1},
        {"Upsample", 1},        {"Scene", 0},          {"Post-processing", 0}, {"Bloom", 1},         {"Tone mapping", 1}};
    CHECK(sections == expected);
    CHECK(profiler.GetFrameGpuMilliseconds() > 0.0);
    profiler.SetEnabled(false);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}
