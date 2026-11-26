#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Application.h>
#include <quasar/core/Base.h>
#include <quasar/core/Layer.h>
#include <quasar/core/Window.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/RenderStatistics.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/Texture.h>
#include <quasar/renderer/VertexArray.h>

#include <doctest/doctest.h>

#include <glad/gl.h>

#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    constexpr std::string_view ColorShader = R"(
#type vertex
#version 450 core

void main()
{
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
}

#type fragment
#version 450 core
layout(location = 0) out vec4 o_Color;

void main()
{
    o_Color = vec4(1.0);
}
)";

    constexpr std::uint32_t PipelineStates = 9;

    struct Objects
    {
        explicit Objects(Quasar::GraphicsContext& context)
        {
            const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::Parse("Color", ColorShader);
            REQUIRE(specification.has_value());
            Shader = context.CreateShader(*specification);
            REQUIRE(Shader != nullptr);
            Pipeline = context.CreatePipeline({.Shader = Shader.get()});
            Framebuffer = context.CreateFramebuffer({.Width = 4, .Height = 4});
            Texture = context.CreateTexture2D({.Width = 1, .Height = 1, .Mipmaps = false}, {});
            Uniforms = context.CreateUniformBuffer(16, 5);
            REQUIRE(Pipeline != nullptr);
            REQUIRE(Framebuffer != nullptr);
            REQUIRE(Texture != nullptr);
            REQUIRE(Uniforms != nullptr);
        }

        void BindAll() const
        {
            Pipeline->Bind();
            Framebuffer->Bind();
            Texture->Bind(2);
            Uniforms->Bind();
        }

        Quasar::Scope<Quasar::Shader> Shader;
        Quasar::Scope<Quasar::Pipeline> Pipeline;
        Quasar::Scope<Quasar::Framebuffer> Framebuffer;
        Quasar::Scope<Quasar::Texture2D> Texture;
        Quasar::Scope<Quasar::UniformBuffer> Uniforms;
    };

    GLint GetInteger(GLenum name)
    {
        GLint value = 0;
        glGetIntegerv(name, &value);
        return value;
    }

    GLint GetTextureOfSlot(GLuint slot)
    {
        GLint texture = 0;
        glGetIntegeri_v(GL_TEXTURE_BINDING_2D, slot, &texture);
        return texture;
    }

    class BindingLayer : public Quasar::Layer
    {
    public:
        BindingLayer() : Layer("Binding") {}

        void OnAttach() override { m_Objects.emplace(GetContext()); }

        void OnDetach() override { m_Objects.reset(); }

        void OnRender() override
        {
            m_SeenChanges.push_back(GetContext().GetFrameStatistics().StateChanges);
            m_Objects->Pipeline->Bind();
            if (m_SeenChanges.size() == 3)
            {
                Quasar::Application::Get().Close();
            }
        }

        const std::vector<std::uint32_t>& GetSeenChanges() const { return m_SeenChanges; }

    private:
        static Quasar::GraphicsContext& GetContext() { return *Quasar::Application::Get().GetWindow().GetGraphicsContext(); }

        std::optional<Objects> m_Objects;
        std::vector<std::uint32_t> m_SeenChanges;
    };

    std::vector<std::uint32_t> RenderFrames(bool imgui)
    {
        const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateApplicationWithContext({.ImGui = {.Enabled = imgui}});
        REQUIRE(application != nullptr);
        Quasar::Scope<BindingLayer> layer = Quasar::CreateScope<BindingLayer>();
        const BindingLayer& added = *layer;
        application->PushLayer(std::move(layer));
        application->Run();
        return added.GetSeenChanges();
    }
}

TEST_CASE("The context sends a bind or a change only when it differs from what is in place, until InvalidateState")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    Quasar::GraphicsContext& context = *window->GetGraphicsContext();
    const Objects objects(context);
    context.EndFrame();

    objects.BindAll();
    objects.BindAll();
    context.EndFrame();
    CHECK(context.GetFrameStatistics() ==
          Quasar::RenderStatistics{.PipelineBinds = 2, .FramebufferBinds = 1, .TextureBinds = 1, .UniformBufferBinds = 1, .StateChanges = PipelineStates + 1});

    objects.BindAll();
    context.EndFrame();
    CHECK(context.GetFrameStatistics() == Quasar::RenderStatistics{.PipelineBinds = 1});

    context.InvalidateState();
    objects.BindAll();
    context.EndFrame();
    CHECK(context.GetFrameStatistics() ==
          Quasar::RenderStatistics{.PipelineBinds = 1, .FramebufferBinds = 1, .TextureBinds = 1, .UniformBufferBinds = 1, .StateChanges = PipelineStates + 1});
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("After InvalidateState, the binds undo the changes made by direct OpenGL calls")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    Quasar::GraphicsContext& context = *window->GetGraphicsContext();
    const Objects objects(context);
    objects.BindAll();
    const GLint program = GetInteger(GL_CURRENT_PROGRAM);
    const GLint framebuffer = GetInteger(GL_DRAW_FRAMEBUFFER_BINDING);
    REQUIRE(program != 0);
    REQUIRE(framebuffer != 0);

    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBindTextureUnit(2, 0);
    context.InvalidateState();
    objects.BindAll();

    CHECK(GetInteger(GL_CURRENT_PROGRAM) == program);
    CHECK(GetInteger(GL_DRAW_FRAMEBUFFER_BINDING) == framebuffer);
    CHECK(glIsEnabled(GL_DEPTH_TEST) == GL_TRUE);
    CHECK(GetInteger(GL_DEPTH_WRITEMASK) == GL_TRUE);
    CHECK(GetTextureOfSlot(2) != 0);
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("A new object that takes the name of a deleted one is bound, even where the deleted one was bound")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    Quasar::GraphicsContext& context = *window->GetGraphicsContext();
    std::optional<Objects> objects(std::in_place, context);
    Quasar::Scope<Quasar::VertexArray> vertexArray = context.CreateVertexArray();
    REQUIRE(vertexArray != nullptr);
    objects->BindAll();
    context.Draw(*vertexArray, 3);
    objects.reset();
    vertexArray.reset();
    const Objects replaced(context);
    const Quasar::Scope<Quasar::VertexArray> replacedVertexArray = context.CreateVertexArray();
    REQUIRE(replacedVertexArray != nullptr);

    replaced.BindAll();
    context.Draw(*replacedVertexArray, 3);

    CHECK(GetInteger(GL_CURRENT_PROGRAM) != 0);
    CHECK(GetInteger(GL_DRAW_FRAMEBUFFER_BINDING) != 0);
    CHECK(GetTextureOfSlot(2) != 0);
    CHECK(GetInteger(GL_VERTEX_ARRAY_BINDING) != 0);
    GLint uniforms = 0;
    glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 5, &uniforms);
    CHECK(uniforms != 0);
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Without ImGui, the context keeps what is in place from one frame to the next, and with ImGui it binds again after it")
{
    const TestHelpers::ScopedLog log;

    const std::vector<std::uint32_t> withoutImGui = RenderFrames(false);
    REQUIRE(withoutImGui.size() == 3);
    CHECK(withoutImGui[1] > 0);
    CHECK(withoutImGui[2] == 0);

    const std::vector<std::uint32_t> withImGui = RenderFrames(true);
    REQUIRE(withImGui.size() == 3);
    CHECK(withImGui[1] > 0);
    CHECK(withImGui[2] == withImGui[1]);
}
