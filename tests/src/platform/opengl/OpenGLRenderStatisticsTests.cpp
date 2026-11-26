#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Application.h>
#include <quasar/core/Base.h>
#include <quasar/core/Layer.h>
#include <quasar/core/Window.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Mesh.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/Primitives.h>
#include <quasar/renderer/RenderStatistics.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/Texture.h>

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    constexpr std::string_view ValueShader = R"(
#type vertex
#version 450 core
layout(location = 0) in vec3 a_Position;

void main()
{
    gl_Position = vec4(a_Position, 1.0);
}

#type fragment
#version 450 core
uniform float u_Value;
layout(location = 0) out vec4 o_Color;

void main()
{
    o_Color = vec4(u_Value);
}
)";

    struct Drawing
    {
        explicit Drawing(Quasar::GraphicsContext& context)
        {
            const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::Parse("Value", ValueShader);
            REQUIRE(specification.has_value());
            Shader = context.CreateShader(*specification);
            REQUIRE(Shader != nullptr);
            Pipeline = context.CreatePipeline({.Shader = Shader.get(), .DepthTest = false});
            Quad = Quasar::Mesh::Create(context, Quasar::Primitives::CreateQuad());
            REQUIRE(Pipeline != nullptr);
            REQUIRE(Quad != nullptr);
        }

        Quasar::Scope<Quasar::Shader> Shader;
        Quasar::Scope<Quasar::Pipeline> Pipeline;
        Quasar::Scope<Quasar::Mesh> Quad;
    };

    class DrawingLayer : public Quasar::Layer
    {
    public:
        DrawingLayer() : Layer("Drawing") {}

        void OnAttach() override { m_Drawing.emplace(GetContext()); }

        void OnDetach() override { m_Drawing.reset(); }

        void OnRender() override
        {
            Quasar::GraphicsContext& context = GetContext();
            m_SeenDraws.push_back(context.GetFrameStatistics().Draws);
            context.BindWindow();
            m_Drawing->Pipeline->Bind();
            context.DrawIndexed(m_Drawing->Quad->GetVertexArray());
            context.DrawIndexed(m_Drawing->Quad->GetVertexArray());
            if (m_SeenDraws.size() == 3)
            {
                Quasar::Application::Get().Close();
            }
        }

        const std::vector<std::uint32_t>& GetSeenDraws() const { return m_SeenDraws; }

    private:
        static Quasar::GraphicsContext& GetContext() { return *Quasar::Application::Get().GetWindow().GetGraphicsContext(); }

        std::optional<Drawing> m_Drawing;
        std::vector<std::uint32_t> m_SeenDraws;
    };
}

TEST_CASE("The statistics of a frame count its draws, its binds and its uploads, and the next frame starts from zero")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    Quasar::GraphicsContext& context = *window->GetGraphicsContext();
    const Drawing drawing(context);
    const Quasar::Scope<Quasar::Framebuffer> framebuffer = context.CreateFramebuffer({.Width = 4, .Height = 4});
    const Quasar::Scope<Quasar::Texture2D> texture = context.CreateTexture2D({.Width = 1, .Height = 1, .Mipmaps = false}, {});
    const Quasar::Scope<Quasar::UniformBuffer> uniforms = context.CreateUniformBuffer(32, 5);
    REQUIRE(framebuffer != nullptr);
    REQUIRE(texture != nullptr);
    REQUIRE(uniforms != nullptr);
    context.EndFrame();

    framebuffer->Bind();
    drawing.Pipeline->Bind();
    drawing.Shader->SetFloat("u_Value", 0.5f);
    drawing.Shader->SetFloat("u_Missing", 0.5f);
    texture->Bind(0);
    framebuffer->BindColorTexture(0, 1);
    constexpr std::array<std::byte, 16> values{};
    uniforms->SetData(values);
    uniforms->SetData(std::span(values).first(4), 16);
    uniforms->Bind();
    constexpr std::array<std::uint8_t, 4> pixel{255, 255, 255, 255};
    const Quasar::Scope<Quasar::Texture2D> uploaded = context.CreateTexture2D({.Width = 1, .Height = 1, .Mipmaps = false}, std::as_bytes(std::span(pixel)));
    REQUIRE(uploaded != nullptr);
    context.DrawIndexed(drawing.Quad->GetVertexArray());
    context.DrawIndexed(drawing.Quad->GetVertexArray());
    framebuffer->Unbind();
    context.BindWindow();
    CHECK(context.GetFrameStatistics() == Quasar::RenderStatistics{});

    context.EndFrame();
    CHECK(context.GetFrameStatistics() == Quasar::RenderStatistics{.Draws = 2,
                                                                   .Triangles = 4,
                                                                   .PipelineBinds = 1,
                                                                   .FramebufferBinds = 3,
                                                                   .TextureBinds = 2,
                                                                   .UniformBufferBinds = 1,
                                                                   .UniformChanges = 1,
                                                                   .Uploads = 3,
                                                                   .UploadedBytes = 24});
    context.EndFrame();
    CHECK(context.GetFrameStatistics() == Quasar::RenderStatistics{});
}

TEST_CASE("The application ends the frame of the statistics after each rendered frame")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateApplicationWithContext();
    REQUIRE(application != nullptr);
    Quasar::Scope<DrawingLayer> layer = Quasar::CreateScope<DrawingLayer>();
    const DrawingLayer& added = *layer;
    application->PushLayer(std::move(layer));

    application->Run();

    CHECK(added.GetSeenDraws() == std::vector<std::uint32_t>{0, 2, 2});
}
