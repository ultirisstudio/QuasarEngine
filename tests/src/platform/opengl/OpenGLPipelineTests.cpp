#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Base.h>
#include <quasar/core/Window.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/VertexArray.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/vec4.hpp>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string_view>

using Quasar::BlendMode;
using Quasar::CullMode;
using Quasar::DepthCompare;
using Quasar::PipelineSpecification;

namespace
{
    // Covers the target with one triangle at the depth u_Depth, whose vertices turn counter-clockwise unless u_Clockwise
    constexpr std::string_view TriangleShader = R"(
#type vertex
#version 450 core
uniform float u_Depth;
uniform bool u_Clockwise;

void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(positions[u_Clockwise ? 2 - gl_VertexID : gl_VertexID], u_Depth, 1.0);
}

#type fragment
#version 450 core
uniform vec4 u_Color;
uniform int u_Value;
layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_Value;

void main()
{
    o_Color = u_Color;
    o_Value = u_Value;
}
)";

    using Color = std::array<int, 4>;

    struct Triangle
    {
        float Depth = 0.5f;
        int Value = 1;
        bool Clockwise = false;
        glm::vec4 Color{1.0f};
    };

    class Scene
    {
    public:
        Scene() : m_Window(TestHelpers::CreateWindowWithContext())
        {
            REQUIRE(m_Window != nullptr);
            m_Shader = CreateShader();
            m_Framebuffer = GetContext().CreateFramebuffer(
                {.Width = 4,
                 .Height = 4,
                 .Attachments = {Quasar::FramebufferFormat::Rgba8, Quasar::FramebufferFormat::RedInteger, Quasar::FramebufferFormat::Depth24Stencil8}});
            m_VertexArray = GetContext().CreateVertexArray();
            REQUIRE(m_Framebuffer != nullptr);
            REQUIRE(m_VertexArray != nullptr);
            Clear();
        }

        Quasar::GraphicsContext& GetContext() const { return *m_Window->GetGraphicsContext(); }
        const Quasar::Window& GetWindow() const { return *m_Window; }
        Quasar::Shader& GetShader() const { return *m_Shader; }
        Quasar::Framebuffer& GetFramebuffer() const { return *m_Framebuffer; }

        Quasar::Scope<Quasar::Shader> CreateShader() const
        {
            const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::Parse("Triangle", TriangleShader);
            REQUIRE(specification.has_value());
            Quasar::Scope<Quasar::Shader> shader = GetContext().CreateShader(*specification);
            REQUIRE(shader != nullptr);
            return shader;
        }

        Quasar::Scope<Quasar::Pipeline> CreatePipeline(PipelineSpecification specification) const
        {
            if (specification.Shader == nullptr)
            {
                specification.Shader = m_Shader.get();
            }
            Quasar::Scope<Quasar::Pipeline> pipeline = GetContext().CreatePipeline(specification);
            REQUIRE(pipeline != nullptr);
            return pipeline;
        }

        void Clear(float depth = 1.0f) const
        {
            m_Framebuffer->ClearColor(0, {0.0f, 0.0f, 1.0f, 1.0f});
            m_Framebuffer->ClearInteger(1, 0);
            m_Framebuffer->ClearDepthStencil(depth);
        }

        int Draw(const Triangle& triangle, Quasar::Shader* shader = nullptr) const
        {
            Quasar::Shader& target = shader != nullptr ? *shader : *m_Shader;
            target.SetFloat("u_Depth", triangle.Depth);
            target.SetInt("u_Value", triangle.Value);
            target.SetInt("u_Clockwise", triangle.Clockwise ? 1 : 0);
            target.SetVec4("u_Color", triangle.Color);
            m_Framebuffer->Bind();
            GetContext().Draw(*m_VertexArray, 3);
            m_Framebuffer->Unbind();
            return m_Framebuffer->ReadInteger(1, 0, 0).value_or(-1);
        }

        void DrawIntoWindow(const Triangle& triangle) const
        {
            m_Shader->SetFloat("u_Depth", triangle.Depth);
            m_Shader->SetInt("u_Clockwise", 0);
            m_Shader->SetVec4("u_Color", triangle.Color);
            GetContext().Draw(*m_VertexArray, 3);
        }

        Color ReadColor() const
        {
            std::array<std::uint8_t, 4> pixel{};
            m_Framebuffer->Bind();
            glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
            m_Framebuffer->Unbind();
            return {pixel[0], pixel[1], pixel[2], pixel[3]};
        }

    private:
        Quasar::Scope<Quasar::Window> m_Window;
        Quasar::Scope<Quasar::Shader> m_Shader;
        Quasar::Scope<Quasar::Framebuffer> m_Framebuffer;
        Quasar::Scope<Quasar::VertexArray> m_VertexArray;
    };

    bool IsNear(const Color& color, const Color& expected)
    {
        for (std::size_t channel = 0; channel < color.size(); ++channel)
        {
            if (std::abs(color.at(channel) - expected.at(channel)) > 1)
            {
                return false;
            }
        }
        return true;
    }

    Color ReadWindowPixel(int x, int y)
    {
        std::array<std::uint8_t, 4> pixel{};
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
        return {pixel[0], pixel[1], pixel[2], pixel[3]};
    }
}

TEST_CASE("Bind makes the draws use the shader of the pipeline")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;
    const Quasar::Scope<Quasar::Shader> other = scene.CreateShader();
    const Quasar::Scope<Quasar::Pipeline> first = scene.CreatePipeline({});
    const Quasar::Scope<Quasar::Pipeline> second = scene.CreatePipeline({.Shader = other.get(), .DepthTest = false});
    scene.GetShader().SetInt("u_Value", 1);
    other->SetInt("u_Value", 2);

    second->Bind();
    CHECK(scene.Draw({.Value = 2}, other.get()) == 2);
    first->Bind();
    scene.Clear();
    CHECK(scene.Draw({.Value = 1}) == 1);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The depth test keeps the closest pixels, with depths from 0 to 1")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;
    const Quasar::Scope<Quasar::Pipeline> tested = scene.CreatePipeline({});
    const Quasar::Scope<Quasar::Pipeline> untested = scene.CreatePipeline({.DepthTest = false});

    tested->Bind();
    CHECK(scene.Draw({.Depth = 0.25f, .Value = 1}) == 1);
    CHECK(scene.Draw({.Depth = 0.75f, .Value = 2}) == 1);
    CHECK(scene.Draw({.Depth = 0.1f, .Value = 3}) == 3);
    CHECK(scene.Draw({.Depth = 0.0f, .Value = 4}) == 4);
    CHECK(scene.Draw({.Depth = -0.5f, .Value = 5}) == 4);
    untested->Bind();
    CHECK(scene.Draw({.Depth = 0.75f, .Value = 6}) == 6);
    CHECK(scene.Draw({.Depth = -0.5f, .Value = 7}) == 6);
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Each depth comparison draws the pixels it accepts")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;

    struct Expected
    {
        DepthCompare Compare = DepthCompare::Less;
        std::array<bool, 3> Drawn{};
    };
    constexpr std::array<Expected, 6> expectations{{
        {DepthCompare::Less, {true, false, false}},
        {DepthCompare::LessOrEqual, {true, true, false}},
        {DepthCompare::Greater, {false, false, true}},
        {DepthCompare::GreaterOrEqual, {false, true, true}},
        {DepthCompare::Equal, {false, true, false}},
        {DepthCompare::Always, {true, true, true}},
    }};
    constexpr std::array<float, 3> depths{0.25f, 0.5f, 0.75f};

    for (const Expected& expected : expectations)
    {
        const Quasar::Scope<Quasar::Pipeline> pipeline = scene.CreatePipeline({.Compare = expected.Compare});
        pipeline->Bind();
        for (std::size_t index = 0; index < depths.size(); ++index)
        {
            CAPTURE(static_cast<int>(expected.Compare));
            CAPTURE(index);
            scene.Clear(0.5f);
            CHECK((scene.Draw({.Depth = depths.at(index), .Value = 1}) == 1) == expected.Drawn.at(index));
        }
    }
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("A pipeline without depth writes keeps the stored depths, which can still be cleared")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;
    const Quasar::Scope<Quasar::Pipeline> readOnly = scene.CreatePipeline({.DepthWrite = false});
    const Quasar::Scope<Quasar::Pipeline> writing = scene.CreatePipeline({});

    readOnly->Bind();
    CHECK(scene.Draw({.Depth = 0.25f, .Value = 1}) == 1);
    writing->Bind();
    CHECK(scene.Draw({.Depth = 0.75f, .Value = 2}) == 2);

    readOnly->Bind();
    scene.Clear(0.5f);
    CHECK(scene.Draw({.Depth = 0.25f, .Value = 3}) == 3);
    writing->Bind();
    CHECK(scene.Draw({.Depth = 0.75f, .Value = 4}) == 3);
    CHECK(scene.Draw({.Depth = 0.4f, .Value = 5}) == 5);
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Culling removes the back, the front or no side of the triangles")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;

    struct Expected
    {
        CullMode Culling = CullMode::None;
        bool CounterClockwise = true;
        bool Clockwise = true;
    };
    for (const Expected& expected : std::array<Expected, 3>{{{CullMode::None, true, true}, {CullMode::Back, true, false}, {CullMode::Front, false, true}}})
    {
        CAPTURE(static_cast<int>(expected.Culling));
        const Quasar::Scope<Quasar::Pipeline> pipeline = scene.CreatePipeline({.DepthTest = false, .Culling = expected.Culling});
        pipeline->Bind();
        scene.Clear();
        CHECK((scene.Draw({.Value = 1, .Clockwise = false}) == 1) == expected.CounterClockwise);
        scene.Clear();
        CHECK((scene.Draw({.Value = 1, .Clockwise = true}) == 1) == expected.Clockwise);
    }
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Blending replaces, lays over or adds the colors")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;
    const Triangle red{.Color = {1.0f, 0.0f, 0.0f, 0.25f}};

    struct Expected
    {
        BlendMode Blending = BlendMode::None;
        Color Result{};
    };
    for (const Expected& expected : std::array<Expected, 3>{{
             {BlendMode::Alpha, {64, 0, 191, 255}},
             {BlendMode::Additive, {64, 0, 255, 255}},
             {BlendMode::None, {255, 0, 0, 64}},
         }})
    {
        CAPTURE(static_cast<int>(expected.Blending));
        const Quasar::Scope<Quasar::Pipeline> pipeline = scene.CreatePipeline({.DepthTest = false, .Blending = expected.Blending});
        pipeline->Bind();
        scene.Clear();
        scene.Draw(red);
        CHECK(IsNear(scene.ReadColor(), expected.Result));
    }
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("BindWindow draws into the whole window, which ClearWindow fills whatever the pipeline")
{
    const TestHelpers::ScopedLog log;
    const Scene scene;
    const auto width = static_cast<int>(scene.GetWindow().GetWidth());
    const auto height = static_cast<int>(scene.GetWindow().GetHeight());
    const Quasar::Scope<Quasar::Pipeline> readOnly = scene.CreatePipeline({.DepthWrite = false});
    const Quasar::Scope<Quasar::Pipeline> writing = scene.CreatePipeline({});

    scene.GetFramebuffer().Bind();
    scene.GetContext().BindWindow();
    readOnly->Bind();
    scene.GetContext().ClearWindow({0.0f, 1.0f, 0.0f, 1.0f}, 0.5f);
    std::array<GLint, 4> viewport{};
    glGetIntegerv(GL_VIEWPORT, viewport.data());
    CHECK(viewport == std::array<GLint, 4>{0, 0, width, height});
    CHECK(IsNear(ReadWindowPixel(width - 1, height - 1), {0, 255, 0, 255}));

    writing->Bind();
    scene.DrawIntoWindow({.Depth = 0.75f, .Color = {1.0f, 0.0f, 0.0f, 1.0f}});
    CHECK(IsNear(ReadWindowPixel(width / 2, height / 2), {0, 255, 0, 255}));
    scene.DrawIntoWindow({.Depth = 0.25f, .Color = {1.0f, 0.0f, 0.0f, 1.0f}});
    CHECK(IsNear(ReadWindowPixel(width - 1, height - 1), {255, 0, 0, 255}));
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("No pipeline is created without a shader")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);

    CHECK(window->GetGraphicsContext()->CreatePipeline({}) == nullptr);
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}
