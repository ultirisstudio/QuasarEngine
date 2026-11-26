#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Base.h>
#include <quasar/core/Window.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Mesh.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/Primitives.h>
#include <quasar/renderer/RenderStatistics.h>
#include <quasar/renderer/Shader.h>

#include <doctest/doctest.h>

#include <glad/gl.h>

#include <optional>
#include <string_view>

namespace
{
    constexpr std::string_view InstanceShader = R"(
#type vertex
#version 450 core
#extension GL_ARB_shader_draw_parameters : require
layout(location = 0) in vec3 a_Position;
layout(location = 0) flat out int v_Value;

void main()
{
    v_Value = (gl_BaseInstanceARB * 100) + gl_InstanceID;
    gl_Position = vec4((a_Position.x * 0.5) - 0.75 + (0.5 * float(gl_InstanceID)), a_Position.y * 2.0, 0.0, 1.0);
}

#type fragment
#version 450 core
layout(location = 0) flat in int v_Value;
layout(location = 0) out int o_Value;

void main()
{
    o_Value = v_Value;
}
)";
}

TEST_CASE("DrawInstances draws each instance of a mesh with its number, from the first instance")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    Quasar::GraphicsContext& context = *window->GetGraphicsContext();
    const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::Parse("Instance", InstanceShader);
    REQUIRE(specification.has_value());
    const Quasar::Scope<Quasar::Shader> shader = context.CreateShader(*specification);
    REQUIRE(shader != nullptr);
    const Quasar::Scope<Quasar::Pipeline> pipeline = context.CreatePipeline({.Shader = shader.get(), .DepthTest = false, .Culling = Quasar::CullMode::None});
    const Quasar::Scope<Quasar::Mesh> quad = Quasar::Mesh::Create(context, Quasar::Primitives::CreateQuad());
    const Quasar::Scope<Quasar::Framebuffer> target =
        context.CreateFramebuffer({.Width = 4, .Height = 1, .Attachments = {Quasar::FramebufferFormat::RedInteger}});
    REQUIRE(pipeline != nullptr);
    REQUIRE(quad != nullptr);
    REQUIRE(target != nullptr);
    target->ClearInteger(0, -1);
    context.EndFrame();

    target->Bind();
    pipeline->Bind();
    context.DrawInstances(quad->GetVertexArray(), 3, 5);
    context.DrawInstances(quad->GetVertexArray(), 0, 9);
    target->Unbind();
    context.EndFrame();

    CHECK(target->ReadInteger(0, 0, 0) == 500);
    CHECK(target->ReadInteger(0, 1, 0) == 501);
    CHECK(target->ReadInteger(0, 2, 0) == 502);
    CHECK(target->ReadInteger(0, 3, 0) == -1);
    CHECK(context.GetFrameStatistics().Draws == 1);
    CHECK(context.GetFrameStatistics().Triangles == 6);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}
