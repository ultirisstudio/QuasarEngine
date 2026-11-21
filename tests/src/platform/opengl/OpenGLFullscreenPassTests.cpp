#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"
#include "helpers/RenderHelpers.h"

#include <quasar/core/Base.h>
#include <quasar/core/Window.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/FullscreenPass.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/Shader.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/vec2.hpp>

#include <array>
#include <optional>
#include <string_view>

using TestHelpers::IsNear;
using TestHelpers::ReadColor;
using TestHelpers::ReadWindowPixel;

namespace
{
    constexpr std::string_view GreenShader = R"(
#type vertex
#version 450 core
void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}

#type fragment
#version 450 core
layout(location = 0) out vec4 o_Color;
void main()
{
    o_Color = vec4(0.0, 1.0, 0.0, 1.0);
}
)";

    bool IsWindowBound(const Quasar::GraphicsContext& context)
    {
        GLint framebuffer = -1;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
        std::array<GLint, 4> viewport{};
        glGetIntegerv(GL_VIEWPORT, viewport.data());
        const glm::uvec2 size = context.GetWindowSize();
        return framebuffer == 0 && viewport[2] == static_cast<GLint>(size.x) && viewport[3] == static_cast<GLint>(size.y);
    }
}

TEST_CASE("A fullscreen pass covers every pixel of its target, then binds the window again")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    Quasar::GraphicsContext& context = *window->GetGraphicsContext();
    const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::Parse("Green", GreenShader);
    REQUIRE(specification.has_value());
    const Quasar::Scope<Quasar::Shader> shader = context.CreateShader(*specification);
    REQUIRE(shader != nullptr);
    const Quasar::Scope<Quasar::Pipeline> pipeline =
        context.CreatePipeline({.Shader = shader.get(), .DepthTest = false, .DepthWrite = false, .Culling = Quasar::CullMode::None});
    const Quasar::Scope<Quasar::Framebuffer> target = context.CreateFramebuffer({.Width = 5, .Height = 3});
    const Quasar::Scope<Quasar::FullscreenPass> pass = Quasar::FullscreenPass::Create(context);
    REQUIRE(pipeline != nullptr);
    REQUIRE(target != nullptr);
    REQUIRE(pass != nullptr);
    CHECK(&pass->GetContext() == &context);
    target->ClearColor(0, {1.0f, 0.0f, 0.0f, 1.0f});
    context.ClearWindow({1.0f, 0.0f, 0.0f, 1.0f});

    target->Bind();
    pass->Draw(*pipeline, target.get());
    CHECK(IsWindowBound(context));
    for (const auto& [x, y] : {std::array{0, 0}, std::array{4, 0}, std::array{0, 2}, std::array{4, 2}, std::array{2, 1}})
    {
        CAPTURE(x);
        CAPTURE(y);
        CHECK(IsNear(ReadColor(*target, x, y), {0, 255, 0, 255}));
    }
    CHECK(IsNear(ReadWindowPixel(0, 0), {255, 0, 0, 255}));

    target->Bind();
    pass->Draw(*pipeline, nullptr);
    CHECK(IsWindowBound(context));
    const glm::uvec2 size = context.GetWindowSize();
    CHECK(IsNear(ReadWindowPixel(0, 0), {0, 255, 0, 255}));
    CHECK(IsNear(ReadWindowPixel(static_cast<int>(size.x) - 1, static_cast<int>(size.y) - 1), {0, 255, 0, 255}));
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}
