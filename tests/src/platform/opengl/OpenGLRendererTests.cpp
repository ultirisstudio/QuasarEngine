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
#include <quasar/renderer/ShaderBindings.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using Quasar::UniformType;

namespace
{
    constexpr std::string_view BlocksShader = R"(
#type vertex
#version 450 core

void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}

#type fragment
#version 450 core
layout(std140, binding = 0) uniform Camera
{
    mat4 u_ViewProjection;
    mat4 u_View;
    mat4 u_Projection;
    vec4 u_CameraPosition;
};
layout(std140, binding = 2) uniform Object
{
    mat4 u_Model;
};
layout(location = 0) out int o_Value;

void main()
{
    const mat4 difference = u_ViewProjection - u_Projection * u_View;
    const bool product = all(lessThan(abs(difference[3]), vec4(1e-4)));
    o_Value = int(round(u_CameraPosition.y)) + 10 * int(round(-u_View[3][2])) + 100 * int(round(u_Model[3][0])) +
              1000 * int(round(u_Projection[1][1])) + (product ? 10000 : 0);
}
)";

    using Color = std::array<int, 4>;

    constexpr glm::vec4 Black{0.0f, 0.0f, 0.0f, 1.0f};
    constexpr Color BlackPixel{0, 0, 0, 255};
    constexpr Color RedPixel{255, 0, 0, 255};
    constexpr Color BluePixel{0, 0, 255, 255};

    class Scene
    {
    public:
        Scene() : m_Window(TestHelpers::CreateWindowWithContext())
        {
            REQUIRE(m_Window != nullptr);
            const std::optional<Quasar::ShaderSpecification> unlit =
                Quasar::ShaderSpecification::FromFile(Quasar::GetEngineResourcesDirectory() / "shaders" / "Unlit.glsl");
            REQUIRE(unlit.has_value());
            m_Shader = GetContext().CreateShader(*unlit);
            REQUIRE(m_Shader != nullptr);
            m_Pipeline = GetContext().CreatePipeline({.Shader = m_Shader.get()});
            m_Framebuffer = GetContext().CreateFramebuffer({.Width = 8, .Height = 8});
            m_Renderer = Quasar::Renderer::Create(GetContext());
            m_Quad = Quasar::Mesh::Create(GetContext(), Quasar::Primitives::CreateQuad());
            REQUIRE(m_Pipeline != nullptr);
            REQUIRE(m_Framebuffer != nullptr);
            REQUIRE(m_Renderer != nullptr);
            REQUIRE(m_Quad != nullptr);
            m_Red = CreateColor({1.0f, 0.0f, 0.0f, 1.0f});
            m_Blue = CreateColor({0.0f, 0.0f, 1.0f, 1.0f});
            m_Camera.SetPerspective({.VerticalFov = glm::radians(90.0f), .Near = 0.1f, .Far = 10.0f});
            m_Camera.SetPosition({0.0f, 0.0f, 1.0f});
            Clear();
        }

        Quasar::GraphicsContext& GetContext() const { return *m_Window->GetGraphicsContext(); }
        const Quasar::Window& GetWindow() const { return *m_Window; }
        Quasar::Renderer& GetRenderer() const { return *m_Renderer; }
        Quasar::Framebuffer& GetFramebuffer() const { return *m_Framebuffer; }
        Quasar::Camera& GetCamera() { return m_Camera; }
        const Quasar::Camera& GetCamera() const { return m_Camera; }
        const Quasar::Mesh& GetQuad() const { return *m_Quad; }
        Quasar::Material& GetRed() const { return *m_Red; }
        Quasar::Material& GetBlue() const { return *m_Blue; }
        Quasar::Pipeline& GetPipeline() const { return *m_Pipeline; }

        void Clear() const
        {
            m_Framebuffer->ClearColor(0, Black);
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
                GetContext(), {.Pipeline = m_Pipeline.get(), .Uniforms = {{"Color", UniformType::Vec4}}, .Textures = {{.Name = "Texture", .Slot = 0}}});
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
        Quasar::Camera m_Camera;
    };

    bool IsNear(const Color& color, const Color& expected)
    {
        return std::ranges::equal(color, expected, [](int value, int other) { return std::abs(value - other) <= 1; });
    }

    Color ReadWindowPixel(int x, int y)
    {
        std::array<std::uint8_t, 4> pixel{};
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
        return {pixel[0], pixel[1], pixel[2], pixel[3]};
    }

    std::ptrdiff_t CountErrors(std::string_view text)
    {
        const std::vector<Quasar::LogMessage> messages = Quasar::Log::GetMemorySink()->GetMessages();
        return std::ranges::count_if(messages, [text](const Quasar::LogMessage& message)
                                     { return message.Level == Quasar::LogLevel::Error && message.Text.find(text) != std::string::npos; });
    }
}

TEST_CASE("The renderer draws a mesh with its material, seen from the camera, then goes back to the window")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;

    scene.GetRenderer().BeginScene(scene.GetCamera(), &scene.GetFramebuffer());
    scene.GetRenderer().Submit(scene.GetQuad(), scene.GetRed(), glm::mat4(1.0f));
    scene.GetRenderer().EndScene();

    GLint framebuffer = -1;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer);
    std::array<GLint, 4> viewport{};
    glGetIntegerv(GL_VIEWPORT, viewport.data());
    CHECK(framebuffer == 0);
    CHECK(viewport == std::array<GLint, 4>{0, 0, static_cast<GLint>(scene.GetWindow().GetWidth()), static_cast<GLint>(scene.GetWindow().GetHeight())});
    CHECK(IsNear(scene.ReadColor(2, 2), RedPixel));
    CHECK(IsNear(scene.ReadColor(5, 5), RedPixel));
    CHECK(IsNear(scene.ReadColor(1, 1), BlackPixel));
    CHECK(IsNear(scene.ReadColor(6, 6), BlackPixel));
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("Each submission places its mesh with its own transform and material")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;
    const glm::mat4 half = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));

    scene.GetRenderer().BeginScene(scene.GetCamera(), &scene.GetFramebuffer());
    scene.GetRenderer().Submit(scene.GetQuad(), scene.GetRed(), glm::translate(glm::mat4(1.0f), {-0.5f, 0.5f, 0.0f}) * half);
    scene.GetRenderer().Submit(scene.GetQuad(), scene.GetBlue(), glm::translate(glm::mat4(1.0f), {0.5f, -0.5f, 0.0f}) * half);
    scene.GetRenderer().EndScene();

    CHECK(IsNear(scene.ReadColor(1, 1), RedPixel));
    CHECK(IsNear(scene.ReadColor(6, 6), BluePixel));
    CHECK(IsNear(scene.ReadColor(6, 1), BlackPixel));
    CHECK(IsNear(scene.ReadColor(1, 6), BlackPixel));
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("The closest mesh stays in front, whatever the order of the submissions")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;
    const glm::mat4 close = glm::scale(glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, 0.5f}), glm::vec3(0.25f));
    const glm::mat4 far = glm::scale(glm::translate(glm::mat4(1.0f), {0.0f, 0.0f, -0.5f}), glm::vec3(4.0f));

    for (const bool closeFirst : {true, false})
    {
        CAPTURE(closeFirst);
        scene.Clear();
        scene.GetRenderer().BeginScene(scene.GetCamera(), &scene.GetFramebuffer());
        if (closeFirst)
        {
            scene.GetRenderer().Submit(scene.GetQuad(), scene.GetBlue(), close);
        }
        scene.GetRenderer().Submit(scene.GetQuad(), scene.GetRed(), far);
        if (!closeFirst)
        {
            scene.GetRenderer().Submit(scene.GetQuad(), scene.GetBlue(), close);
        }
        scene.GetRenderer().EndScene();

        CHECK(IsNear(scene.ReadColor(3, 3), BluePixel));
        CHECK(IsNear(scene.ReadColor(0, 0), RedPixel));
    }
}

TEST_CASE("The renderer draws into the window without a framebuffer")
{
    const TestHelpers::ScopedLog log;
    Scene scene;
    const auto width = static_cast<int>(scene.GetWindow().GetWidth());
    const auto height = static_cast<int>(scene.GetWindow().GetHeight());
    scene.GetCamera().SetViewportSize(scene.GetWindow().GetWidth(), scene.GetWindow().GetHeight());

    scene.GetFramebuffer().Bind();
    scene.GetRenderer().BeginScene(scene.GetCamera());
    scene.GetContext().ClearWindow(Black);
    scene.GetRenderer().Submit(scene.GetQuad(), scene.GetRed(), glm::mat4(1.0f));
    scene.GetRenderer().EndScene();

    CHECK(IsNear(ReadWindowPixel(width / 2, height / 2), RedPixel));
    CHECK(IsNear(ReadWindowPixel(0, 0), BlackPixel));
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("The shaders read the camera and the object from the blocks of the renderer, with any material without values")
{
    const TestHelpers::ScopedLog log;
    Scene scene;
    const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::Parse("Blocks", BlocksShader);
    REQUIRE(specification.has_value());
    const Quasar::Scope<Quasar::Shader> shader = scene.GetContext().CreateShader(*specification);
    REQUIRE(shader != nullptr);
    const Quasar::Scope<Quasar::Pipeline> pipeline = scene.GetContext().CreatePipeline({.Shader = shader.get(), .DepthTest = false});
    REQUIRE(pipeline != nullptr);
    const Quasar::Scope<Quasar::Material> material =
        Quasar::Material::Create(scene.GetContext(), {.Pipeline = pipeline.get(), .UniformBinding = Quasar::ShaderBindings::Object});
    const Quasar::Scope<Quasar::Framebuffer> values =
        scene.GetContext().CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {Quasar::FramebufferFormat::RedInteger}});
    REQUIRE(material != nullptr);
    REQUIRE(values != nullptr);
    scene.GetCamera().SetPosition({0.0f, 3.0f, 5.0f});

    values->ClearInteger(0, -1);
    scene.GetRenderer().BeginScene(scene.GetCamera(), values.get());
    scene.GetRenderer().Submit(scene.GetQuad(), *material, glm::translate(glm::mat4(1.0f), {7.0f, 0.0f, 0.0f}));
    scene.GetRenderer().EndScene();

    CHECK(values->ReadInteger(0, 0, 0) == 11753);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("Inside a scene, the renderer binds a pipeline or a material only when it changes, and each scene binds them again")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;
    const Quasar::Scope<Quasar::Pipeline> culled = scene.GetContext().CreatePipeline(
        {.Shader = scene.GetRed().GetSpecification().Pipeline->GetSpecification().Shader, .Culling = Quasar::CullMode::Front});
    REQUIRE(culled != nullptr);
    const glm::mat4 left = glm::translate(glm::mat4(1.0f), {-0.5f, 0.0f, 0.0f});
    const glm::mat4 right = glm::translate(glm::mat4(1.0f), {0.5f, 0.0f, 0.0f});
    Quasar::Renderer& renderer = scene.GetRenderer();

    renderer.BeginScene(scene.GetCamera(), &scene.GetFramebuffer());
    renderer.Submit(scene.GetQuad(), scene.GetRed(), left);
    scene.GetBlue().BindResources();
    renderer.Submit(scene.GetQuad(), scene.GetRed(), right);
    renderer.EndScene();
    CHECK(IsNear(scene.ReadColor(1, 3), RedPixel));
    CHECK(IsNear(scene.ReadColor(6, 3), BluePixel));

    scene.Clear();
    renderer.BeginScene(scene.GetCamera(), &scene.GetFramebuffer());
    renderer.Submit(scene.GetQuad(), scene.GetRed(), left);
    scene.GetBlue().BindResources();
    scene.GetRed().SetVec4("Color", {1.0f, 0.0f, 0.0f, 1.0f});
    renderer.Submit(scene.GetQuad(), scene.GetRed(), right);
    renderer.EndScene();
    CHECK(IsNear(scene.ReadColor(6, 3), RedPixel));

    scene.Clear();
    renderer.BeginScene(scene.GetCamera(), &scene.GetFramebuffer());
    renderer.Submit(scene.GetQuad(), scene.GetRed(), left);
    culled->Bind();
    renderer.Submit(scene.GetQuad(), scene.GetRed(), right);
    renderer.EndScene();
    CHECK(IsNear(scene.ReadColor(6, 3), BlackPixel));

    scene.Clear();
    renderer.BeginScene(scene.GetCamera(), &scene.GetFramebuffer());
    renderer.Submit(scene.GetQuad(), scene.GetRed(), left);
    renderer.EndScene();
    culled->Bind();
    scene.GetBlue().BindResources();
    renderer.BeginScene(scene.GetCamera(), &scene.GetFramebuffer());
    renderer.Submit(scene.GetQuad(), scene.GetRed(), right);
    renderer.EndScene();
    CHECK(IsNear(scene.ReadColor(1, 3), RedPixel));
    CHECK(IsNear(scene.ReadColor(6, 3), RedPixel));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The renderer refuses draws outside of a scene and scenes out of order")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;

    scene.GetRenderer().Submit(scene.GetQuad(), scene.GetRed(), glm::mat4(1.0f));
    scene.GetRenderer().Submit(scene.GetQuad(), scene.GetPipeline(), glm::mat4(1.0f));
    scene.GetRenderer().EndScene();
    CHECK(CountErrors("outside of a scene") == 2);
    CHECK(CountErrors("did not begin") == 1);

    scene.GetRenderer().BeginScene(scene.GetCamera(), &scene.GetFramebuffer());
    scene.GetRenderer().BeginScene(scene.GetCamera(), nullptr);
    scene.GetRenderer().EndScene();
    CHECK(CountErrors("before the previous one ends") == 1);
    CHECK(IsNear(scene.ReadColor(3, 3), BlackPixel));

    scene.GetRenderer().BeginScene(scene.GetCamera(), &scene.GetFramebuffer());
    scene.GetRenderer().Submit(scene.GetQuad(), scene.GetRed(), glm::mat4(1.0f));
    scene.GetRenderer().EndScene();
    CHECK(IsNear(scene.ReadColor(3, 3), RedPixel));
    CHECK(glGetError() == GL_NO_ERROR);
}
