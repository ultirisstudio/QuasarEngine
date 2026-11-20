#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"
#include "helpers/RenderHelpers.h"

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
#include <quasar/renderer/PipelineLibrary.h>
#include <quasar/renderer/Primitives.h>
#include <quasar/renderer/Renderer.h>
#include <quasar/renderer/Shader.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using Quasar::PipelineLibrary;
using Quasar::UniformType;
using TestHelpers::Color;
using TestHelpers::CountErrors;
using TestHelpers::IsNear;

namespace
{
    class Fixture
    {
    public:
        Fixture() : m_Window(TestHelpers::CreateWindowWithContext())
        {
            REQUIRE(m_Window != nullptr);
            m_Library = Quasar::CreateScope<PipelineLibrary>(GetContext());
            m_Framebuffer = GetContext().CreateFramebuffer({.Width = 8, .Height = 8});
            m_Renderer = Quasar::Renderer::Create(GetContext());
            m_Quad = Quasar::Mesh::Create(GetContext(), Quasar::Primitives::CreateQuad());
            REQUIRE(m_Framebuffer != nullptr);
            REQUIRE(m_Renderer != nullptr);
            REQUIRE(m_Quad != nullptr);
            SetCamera({0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f});
        }

        Quasar::GraphicsContext& GetContext() const { return *m_Window->GetGraphicsContext(); }
        PipelineLibrary& GetLibrary() const { return *m_Library; }

        void SetCamera(const glm::vec3& position, const glm::vec3& target, const glm::vec3& up = {0.0f, 1.0f, 0.0f})
        {
            m_Camera.SetPerspective({.VerticalFov = glm::radians(90.0f), .Near = 0.1f, .Far = 10.0f});
            m_Camera.SetPosition(position);
            m_Camera.LookAt(target, up);
        }

        Color Draw(Quasar::Material& material, const Quasar::SceneLights& lights = {}, const glm::mat4& transform = glm::mat4(1.0f)) const
        {
            m_Framebuffer->ClearColor(0, {0.0f, 0.0f, 0.0f, 1.0f});
            m_Framebuffer->ClearDepthStencil();
            m_Renderer->BeginScene(m_Camera, m_Framebuffer.get(), lights);
            m_Renderer->Submit(*m_Quad, material, transform);
            m_Renderer->EndScene();

            std::array<std::uint8_t, 4> pixel{};
            m_Framebuffer->Bind();
            glReadPixels(4, 3, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
            m_Framebuffer->Unbind();
            return {pixel[0], pixel[1], pixel[2], pixel[3]};
        }

    private:
        Quasar::Scope<Quasar::Window> m_Window;
        Quasar::Scope<PipelineLibrary> m_Library;
        Quasar::Scope<Quasar::Framebuffer> m_Framebuffer;
        Quasar::Scope<Quasar::Renderer> m_Renderer;
        Quasar::Scope<Quasar::Mesh> m_Quad;
        Quasar::Camera m_Camera;
    };

    Color Gray(int value)
    {
        return {value, value, value, 255};
    }

    Quasar::ShaderSpecification ReadShader(std::string_view name)
    {
        const std::optional<Quasar::ShaderSpecification> shader =
            Quasar::ShaderSpecification::FromFile(Quasar::GetEngineResourcesDirectory() / "shaders" / (std::string(name) + ".glsl"));
        REQUIRE(shader.has_value());
        return *shader;
    }

    Quasar::Scope<Quasar::Material> CreateLit(const PipelineLibrary& library)
    {
        Quasar::Scope<Quasar::Material> material = library.CreateMaterial(PipelineLibrary::BlinnPhong);
        REQUIRE(material != nullptr);
        material->SetFloat("Specular", 0.0f);
        return material;
    }

    Quasar::SceneLights Sun(const Quasar::DirectionalLight& light)
    {
        return {.Directional = light};
    }

    Quasar::SceneLights Light(const glm::vec3& direction, float ambient = 0.0f)
    {
        return Sun({.Direction = direction, .Ambient = ambient});
    }
}

TEST_CASE("The default pipelines of a library are Unlit and BlinnPhong, and are added once")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    PipelineLibrary& library = fixture.GetLibrary();
    CHECK(library.Get(PipelineLibrary::Unlit) == nullptr);

    CHECK(library.AddDefaults());

    for (const std::string_view name : {PipelineLibrary::Unlit, PipelineLibrary::BlinnPhong})
    {
        const Quasar::Pipeline* pipeline = library.Get(name);
        REQUIRE(pipeline != nullptr);
        CHECK(pipeline->GetSpecification().Shader != nullptr);
        CHECK(pipeline->GetSpecification().Culling == Quasar::CullMode::Back);
    }
    CHECK(library.Get(PipelineLibrary::Unlit) != library.Get(PipelineLibrary::BlinnPhong));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));

    const Quasar::Pipeline* unlit = library.Get(PipelineLibrary::Unlit);
    CHECK_FALSE(library.AddDefaults());
    CHECK(CountErrors("already has one") == 2);
    CHECK(library.Get(PipelineLibrary::Unlit) == unlit);
}

TEST_CASE("The materials of the default pipelines start white, with a highlight for BlinnPhong")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    REQUIRE(fixture.GetLibrary().AddDefaults());

    const Quasar::Scope<Quasar::Material> unlit = fixture.GetLibrary().CreateMaterial(PipelineLibrary::Unlit);
    REQUIRE(unlit != nullptr);
    CHECK(unlit->GetSpecification().Pipeline == fixture.GetLibrary().Get(PipelineLibrary::Unlit));
    CHECK(IsNear(fixture.Draw(*unlit), Gray(255)));

    const Quasar::Scope<Quasar::Material> lit = fixture.GetLibrary().CreateMaterial(PipelineLibrary::BlinnPhong);
    REQUIRE(lit != nullptr);
    CHECK(IsNear(fixture.Draw(*lit, Light({0.0f, 0.0f, 1.0f}, 1.0f)), Gray(255)));

    lit->SetVec4("Color", {0.0f, 0.0f, 0.0f, 1.0f});
    const Color highlight = fixture.Draw(*lit, Light({0.0f, 0.0f, -1.0f}));
    CHECK(highlight[0] > 100);
    CHECK(highlight[0] < 125);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A pipeline of a library has the state of its definition with the shader of the library")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;

    Quasar::Pipeline* pipeline = fixture.GetLibrary().Add("Two sided", {.Shader = ReadShader("Unlit"),
                                                                        .State = {.Culling = Quasar::CullMode::None},
                                                                        .UniformBinding = 4,
                                                                        .Uniforms = {{"Color", UniformType::Vec4}},
                                                                        .Textures = {{.Name = "Texture", .Slot = 0}}});

    REQUIRE(pipeline != nullptr);
    CHECK(fixture.GetLibrary().Get("Two sided") == pipeline);
    CHECK(pipeline->GetSpecification().Shader != nullptr);
    CHECK(pipeline->GetSpecification().Culling == Quasar::CullMode::None);

    const Quasar::Scope<Quasar::Material> material = fixture.GetLibrary().CreateMaterial("Two sided");
    REQUIRE(material != nullptr);
    CHECK(material->GetSpecification().Pipeline == pipeline);
    CHECK(material->GetSpecification().UniformBinding == 4);
    CHECK(material->GetSpecification().Uniforms.GetMembers().size() == 1);
    CHECK(material->GetSpecification().Textures.size() == 1);
    CHECK(IsNear(fixture.Draw(*material), {0, 0, 0, 0}));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A library refuses a pipeline without a name, with a name already used or whose shader cannot be created")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    PipelineLibrary& library = fixture.GetLibrary();
    Quasar::Pipeline* first = library.Add("Unlit copy", {.Shader = ReadShader("Unlit")});
    REQUIRE(first != nullptr);

    CHECK(library.Add("", {.Shader = ReadShader("Unlit")}) == nullptr);
    CHECK(CountErrors("without a name") == 1);
    CHECK(library.Add("Unlit copy", {.Shader = ReadShader("BlinnPhong")}) == nullptr);
    CHECK(CountErrors("already has one") == 1);
    CHECK(library.Get("Unlit copy") == first);

    const std::optional<Quasar::ShaderSpecification> broken = Quasar::ShaderSpecification::Parse(
        "Broken", "#type vertex\n#version 450 core\nvoid main() { broken }\n#type fragment\n#version 450 core\nvoid main() {}\n");
    REQUIRE(broken.has_value());
    CHECK(library.Add("Broken", {.Shader = *broken}) == nullptr);
    CHECK(CountErrors("its shader cannot be created") == 1);
    CHECK(library.Get("Broken") == nullptr);
}

TEST_CASE("A library cannot create a material of a pipeline it does not have")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;

    CHECK(fixture.GetLibrary().CreateMaterial("Missing") == nullptr);
    CHECK(CountErrors("has none of this name") == 1);
}

TEST_CASE("The BlinnPhong pipeline lights a surface by the angle of the light, plus its ambient part")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    REQUIRE(fixture.GetLibrary().AddDefaults());
    const Quasar::Scope<Quasar::Material> lit = CreateLit(fixture.GetLibrary());

    CHECK(IsNear(fixture.Draw(*lit, Light({0.0f, 0.0f, -1.0f})), Gray(255)));
    CHECK(IsNear(fixture.Draw(*lit, Light({0.0f, 0.0f, -4.0f})), Gray(255)));
    CHECK(IsNear(fixture.Draw(*lit, Light({0.0f, -std::sqrt(3.0f) / 2.0f, -0.5f})), Gray(128)));
    CHECK(IsNear(fixture.Draw(*lit, Light({0.0f, 0.0f, 1.0f})), Gray(0)));
    CHECK(IsNear(fixture.Draw(*lit, Light({0.0f, 0.0f, 1.0f}, 0.25f)), Gray(64)));
    CHECK(IsNear(fixture.Draw(*lit, Light({0.0f, -std::sqrt(3.0f) / 2.0f, -0.5f}, 0.25f)), Gray(191)));

    lit->SetVec4("Color", {0.0f, 0.0f, 0.0f, 1.0f});
    lit->SetFloat("Specular", 1.0f);
    lit->SetFloat("Shininess", 1.0f);
    CHECK(fixture.Draw(*lit, Light({0.0f, -0.6f, -0.8f}))[0] > 200);
    CHECK(IsNear(fixture.Draw(*lit, Light({0.0f, 0.6f, 0.8f})), Gray(0)));
}

TEST_CASE("The BlinnPhong pipeline uses the color and the intensity of the light, and the color of the material, both converted to linear")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    REQUIRE(fixture.GetLibrary().AddDefaults());
    const Quasar::Scope<Quasar::Material> lit = CreateLit(fixture.GetLibrary());

    CHECK(
        IsNear(fixture.Draw(*lit, Sun({.Direction = {0.0f, 0.0f, -1.0f}, .Color = {1.0f, 0.0f, 0.0f}, .Intensity = 0.5f, .Ambient = 0.0f})), {128, 0, 0, 255}));

    lit->SetVec4("Color", {0.0f, 1.0f, 0.5f, 0.5f});
    CHECK(IsNear(fixture.Draw(*lit, Light({0.0f, 0.0f, -1.0f})), {0, 255, 55, 128}));

    lit->SetVec4("Color", glm::vec4(1.0f));
    CHECK(IsNear(fixture.Draw(*lit, Sun({.Direction = {0.0f, 0.0f, -1.0f}, .Color = glm::vec3(0.5f), .Ambient = 0.0f})), Gray(55)));
}

TEST_CASE("The BlinnPhong pipeline turns the normals with the mesh, and a zero light direction goes down")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    REQUIRE(fixture.GetLibrary().AddDefaults());
    const Quasar::Scope<Quasar::Material> lit = CreateLit(fixture.GetLibrary());

    const glm::mat4 facingBack = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    fixture.SetCamera({0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f});
    CHECK(IsNear(fixture.Draw(*lit, Light({0.0f, 0.0f, 1.0f}), facingBack), Gray(255)));
    CHECK(IsNear(fixture.Draw(*lit, Light({0.0f, 0.0f, -1.0f}), facingBack), Gray(0)));

    const glm::mat4 stretched = glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 1.0f, 1.0f)), glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    fixture.SetCamera({0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f});
    CHECK(IsNear(fixture.Draw(*lit, Light({0.0f, 0.0f, -1.0f}), stretched), Gray(228)));

    const glm::mat4 facingUp = glm::scale(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3(3.0f, 1.0f, 1.0f));
    fixture.SetCamera({0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f});
    CHECK(IsNear(fixture.Draw(*lit, Light({0.0f, 0.0f, 0.0f}), facingUp), Gray(255)));
    CHECK(IsNear(fixture.Draw(*lit, Light({1.0f, -1.0f, 0.0f}), facingUp), Gray(180)));
}

TEST_CASE("A point light lights a surface by its intensity over the square of the distance, until its range")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    REQUIRE(fixture.GetLibrary().AddDefaults());
    const Quasar::Scope<Quasar::Material> lit = CreateLit(fixture.GetLibrary());
    const auto point = [](const Quasar::PointLight& light) { return Quasar::SceneLights{.Directional = std::nullopt, .Points = {light}}; };

    CHECK(IsNear(fixture.Draw(*lit, point({.Position = {0.0f, 0.0f, 1.0f}})), Gray(243)));
    CHECK(IsNear(fixture.Draw(*lit, point({.Position = {0.0f, 0.0f, 2.0f}, .Intensity = 4.0f})), Gray(251)));
    CHECK(IsNear(fixture.Draw(*lit, point({.Position = {0.0f, 0.0f, 2.0f}, .Intensity = 4.0f, .Range = 3.0f})), Gray(161)));
    CHECK(IsNear(fixture.Draw(*lit, point({.Position = {0.0f, 0.0f, 2.0f}, .Intensity = 4.0f, .Range = 2.0f})), Gray(0)));
    CHECK(IsNear(fixture.Draw(*lit, point({.Position = {0.0f, 0.0f, 1.0f}, .Color = {1.0f, 0.0f, 0.0f}, .Intensity = 0.5f})), {122, 0, 0, 255}));
    CHECK(IsNear(fixture.Draw(*lit, point({.Position = {0.0f, 0.0f, -1.0f}})), Gray(0)));
    CHECK(IsNear(fixture.Draw(*lit, Quasar::SceneLights{.Directional = std::nullopt}), Gray(0)));
}

TEST_CASE("A spot light lights only inside its cone, fully inside its inner angle")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    REQUIRE(fixture.GetLibrary().AddDefaults());
    const Quasar::Scope<Quasar::Material> lit = CreateLit(fixture.GetLibrary());
    const auto spot = [](const glm::vec3& direction, float inner, float outer)
    {
        return Quasar::SceneLights{
            .Directional = std::nullopt,
            .Spots = {{.Position = {0.0f, 0.0f, 1.0f}, .Direction = direction, .InnerAngle = glm::radians(inner), .OuterAngle = glm::radians(outer)}}};
    };

    CHECK(IsNear(fixture.Draw(*lit, spot({0.0f, 0.0f, -1.0f}, 20.0f, 30.0f)), Gray(243)));
    CHECK(IsNear(fixture.Draw(*lit, spot({0.0f, 0.0f, -3.0f}, 20.0f, 30.0f)), Gray(243)));
    CHECK(IsNear(fixture.Draw(*lit, spot({0.0f, 0.0f, 0.0f}, 20.0f, 30.0f)), Gray(243)));
    CHECK(IsNear(fixture.Draw(*lit, spot({0.0f, 0.0f, 1.0f}, 20.0f, 30.0f)), Gray(0)));
    CHECK(IsNear(fixture.Draw(*lit, spot({0.0f, 0.0f, -1.0f}, 5.0f, 15.0f)), Gray(94)));
    CHECK(IsNear(fixture.Draw(*lit, spot({0.0f, 0.0f, -1.0f}, 2.0f, 8.0f)), Gray(0)));
    CHECK(IsNear(fixture.Draw(*lit, spot({0.0f, 0.0f, -1.0f}, 40.0f, 30.0f)), Gray(243)));
}

TEST_CASE("The point and spot lights add up with the directional light")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    REQUIRE(fixture.GetLibrary().AddDefaults());
    const Quasar::Scope<Quasar::Material> lit = CreateLit(fixture.GetLibrary());
    Quasar::SceneLights lights = Sun({.Direction = {0.0f, 0.0f, -1.0f}, .Intensity = 0.25f, .Ambient = 0.0f});
    lights.Points.push_back({.Position = {0.0f, 0.0f, 1.0f}, .Intensity = 0.25f});
    CHECK(IsNear(fixture.Draw(*lit, lights), Gray(125)));

    lights.Spots.push_back({.Position = {0.0f, 0.0f, 1.0f}, .Intensity = 0.25f});
    CHECK(IsNear(fixture.Draw(*lit, lights), Gray(185)));
}

TEST_CASE("A scene uses only its first lights up to the maximums, and warns once until it has few enough lights again")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    REQUIRE(fixture.GetLibrary().AddDefaults());
    const Quasar::Scope<Quasar::Material> lit = CreateLit(fixture.GetLibrary());
    const auto countWarnings = []
    {
        const std::vector<Quasar::LogMessage> messages = Quasar::Log::GetMemorySink()->GetMessages();
        return std::ranges::count_if(messages, [](const Quasar::LogMessage& message) { return message.Level == Quasar::LogLevel::Warning; });
    };

    Quasar::SceneLights points{.Directional = std::nullopt,
                               .Points = std::vector<Quasar::PointLight>(32, {.Position = {0.0f, 0.0f, 1.0f}, .Intensity = 0.01f})};
    points.Points.push_back({.Position = {0.0f, 0.0f, 1.0f}, .Intensity = 100.0f});
    CHECK(IsNear(fixture.Draw(*lit, points), Gray(78)));
    CHECK(IsNear(fixture.Draw(*lit, points), Gray(78)));
    CHECK(countWarnings() == 1);

    Quasar::SceneLights spots{.Directional = std::nullopt, .Spots = std::vector<Quasar::SpotLight>(32, {.Position = {0.0f, 0.0f, 1.0f}, .Intensity = 0.01f})};
    CHECK(IsNear(fixture.Draw(*lit, spots), Gray(78)));
    CHECK(countWarnings() == 1);
    spots.Spots.push_back({.Position = {0.0f, 0.0f, 1.0f}, .Intensity = 100.0f});
    CHECK(IsNear(fixture.Draw(*lit, spots), Gray(78)));
    CHECK(countWarnings() == 2);
}
