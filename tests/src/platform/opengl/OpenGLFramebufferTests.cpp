#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"
#include "helpers/TestLayers.h"

#include <quasar/core/Application.h>
#include <quasar/core/Base.h>
#include <quasar/core/Layer.h>
#include <quasar/core/Window.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/VertexArray.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/vec3.hpp>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using Quasar::FramebufferFormat;
using Quasar::FramebufferSpecification;

namespace
{
    Quasar::Scope<Quasar::Framebuffer> CreateFramebuffer(const Quasar::Window& window, const FramebufferSpecification& specification)
    {
        return window.GetGraphicsContext()->CreateFramebuffer(specification);
    }

    void ClearScissored(Quasar::Framebuffer& framebuffer, std::uint32_t attachment, int value, std::array<GLint, 4> area)
    {
        glEnable(GL_SCISSOR_TEST);
        glScissor(area[0], area[1], area[2], area[3]);
        framebuffer.ClearInteger(attachment, value);
        glDisable(GL_SCISSOR_TEST);
    }

    class ImageLayer : public Quasar::Layer
    {
    public:
        explicit ImageLayer(Quasar::Framebuffer& framebuffer) : Layer("Image"), m_Framebuffer(&framebuffer) {}

        void OnImGuiRender() override
        {
            ImGui::Begin("Framebuffer");
            ImGui::Image(m_Framebuffer->GetImGuiTexture(0), ImVec2(64.0f, 32.0f), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
            ImGui::End();
        }

    private:
        Quasar::Framebuffer* m_Framebuffer;
    };
}

TEST_CASE("A framebuffer takes the draws on its whole area between Bind and Unbind")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    const FramebufferSpecification specification{
        .Width = 64, .Height = 32, .Attachments = {FramebufferFormat::Rgba8, FramebufferFormat::RedInteger, FramebufferFormat::Depth24Stencil8}};
    const Quasar::Scope<Quasar::Framebuffer> framebuffer = CreateFramebuffer(*window, specification);
    REQUIRE(framebuffer != nullptr);

    framebuffer->Bind();
    GLint bound = 0;
    std::array<GLint, 4> viewport{};
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &bound);
    glGetIntegerv(GL_VIEWPORT, viewport.data());
    framebuffer->ClearColor(0, {1.0f, 0.5f, 0.25f, 1.0f});
    framebuffer->ClearInteger(1, 5);
    framebuffer->ClearDepthStencil();
    framebuffer->Unbind();
    GLint unbound = -1;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &unbound);

    CHECK(bound != 0);
    CHECK(viewport == std::array<GLint, 4>{0, 0, 64, 32});
    CHECK(unbound == 0);
    CHECK(framebuffer->GetSpecification().Attachments == specification.Attachments);
    CHECK(framebuffer->ReadInteger(1, 63, 31) == 5);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("An integer attachment is read with its rows counted from the top")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    const Quasar::Scope<Quasar::Framebuffer> framebuffer =
        CreateFramebuffer(*window, {.Width = 4, .Height = 3, .Attachments = {FramebufferFormat::RedInteger}});
    REQUIRE(framebuffer != nullptr);

    framebuffer->ClearInteger(0, 1);
    ClearScissored(*framebuffer, 0, 2, {0, 0, 4, 1});
    ClearScissored(*framebuffer, 0, 3, {3, 0, 1, 3});

    CHECK(framebuffer->ReadInteger(0, 0, 0) == 1);
    CHECK(framebuffer->ReadInteger(0, 0, 2) == 2);
    CHECK(framebuffer->ReadInteger(0, 3, 0) == 3);
    CHECK(framebuffer->ReadInteger(0, 4, 0) == std::nullopt);
    CHECK(framebuffer->ReadInteger(0, 0, 3) == std::nullopt);
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Unbind resolves every pixel of a multisampled framebuffer, even with a scissor")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    const Quasar::Scope<Quasar::Framebuffer> framebuffer =
        CreateFramebuffer(*window, {.Width = 8, .Height = 8, .Attachments = {FramebufferFormat::RedInteger, FramebufferFormat::Rgba8}, .Samples = 4});
    REQUIRE(framebuffer != nullptr);

    framebuffer->Bind();
    framebuffer->ClearInteger(0, 9);
    framebuffer->ClearColor(1, {0.0f, 1.0f, 0.0f, 1.0f});
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, 1, 1);
    framebuffer->Unbind();
    const GLboolean scissorAfterUnbind = glIsEnabled(GL_SCISSOR_TEST);
    glDisable(GL_SCISSOR_TEST);

    CHECK(framebuffer->ReadInteger(0, 0, 0) == 9);
    CHECK(framebuffer->ReadInteger(0, 7, 7) == 9);
    CHECK(framebuffer->GetImGuiTexture(1) != ImTextureID{});
    CHECK(scissorAfterUnbind == GL_TRUE);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("Resizing a framebuffer changes its size and keeps its formats, and a size of 0 is ignored")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    const Quasar::Scope<Quasar::Framebuffer> framebuffer =
        CreateFramebuffer(*window, {.Width = 8, .Height = 8, .Attachments = {FramebufferFormat::RedInteger, FramebufferFormat::Rgba8}});
    REQUIRE(framebuffer != nullptr);

    framebuffer->Resize(16, 4);
    framebuffer->Resize(0, 10);
    framebuffer->Resize(10, 0);
    framebuffer->ClearInteger(0, 3);

    CHECK(framebuffer->GetSpecification().Width == 16);
    CHECK(framebuffer->GetSpecification().Height == 4);
    CHECK(framebuffer->ReadInteger(0, 15, 3) == 3);
    CHECK(framebuffer->ReadInteger(0, 0, 4) == std::nullopt);
    CHECK(framebuffer->GetImGuiTexture(1) != ImTextureID{});
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("A framebuffer refuses the attachments that do not exist or that have another format")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    const Quasar::Scope<Quasar::Framebuffer> framebuffer =
        CreateFramebuffer(*window, {.Width = 4, .Height = 4, .Attachments = {FramebufferFormat::Rgba8, FramebufferFormat::RedInteger}});
    REQUIRE(framebuffer != nullptr);
    framebuffer->ClearInteger(1, 4);

    framebuffer->ClearColor(1, {1.0f, 1.0f, 1.0f, 1.0f});
    framebuffer->ClearInteger(0, 7);
    framebuffer->ClearInteger(2, 7);
    framebuffer->ClearDepthStencil();

    CHECK(framebuffer->ReadInteger(1, 0, 0) == 4);
    CHECK(framebuffer->ReadInteger(0, 0, 0) == std::nullopt);
    CHECK(framebuffer->GetImGuiTexture(1) == ImTextureID{});
    CHECK(framebuffer->GetImGuiTexture(2) == ImTextureID{});
    framebuffer->BindColorTexture(1, 0);
    framebuffer->BindColorTexture(2, 0);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    const std::vector<Quasar::LogMessage> messages = Quasar::Log::GetMemorySink()->GetMessages();
    CHECK(std::ranges::count_if(messages, [](const Quasar::LogMessage& message) { return message.Text.starts_with("Cannot bind"); }) == 2);
}

TEST_CASE("A framebuffer can have only a depth attachment")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    const Quasar::Scope<Quasar::Framebuffer> framebuffer =
        CreateFramebuffer(*window, {.Width = 16, .Height = 16, .Attachments = {FramebufferFormat::Depth24Stencil8}, .Samples = 4});
    REQUIRE(framebuffer != nullptr);

    framebuffer->Bind();
    framebuffer->ClearDepthStencil(0.5f, 1);
    framebuffer->Unbind();

    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("No framebuffer is created from a specification that cannot be drawn into")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    const std::array<FramebufferSpecification, 9> specifications{{
        {.Width = 0},
        {.Height = 0},
        {.Attachments = {}},
        {.Attachments = {FramebufferFormat::Depth24Stencil8, FramebufferFormat::Depth24Stencil8}},
        {.Attachments = {FramebufferFormat::Depth24Stencil8, FramebufferFormat::Depth32F}},
        {.Attachments = {FramebufferFormat::Depth32F}, .Samples = 4},
        {.Samples = 0},
        {.Samples = 1024},
        {.Width = 1'000'000},
    }};

    for (const FramebufferSpecification& specification : specifications)
    {
        CAPTURE(specification.Width);
        CAPTURE(specification.Height);
        CAPTURE(specification.Samples);
        CHECK(CreateFramebuffer(*window, specification) == nullptr);
    }
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("ImGui draws a color attachment of a framebuffer")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateApplicationWithContext();
    REQUIRE(application != nullptr);
    ImGui::GetIO().IniFilename = nullptr;
    const Quasar::Scope<Quasar::Framebuffer> framebuffer =
        CreateFramebuffer(application->GetWindow(), {.Width = 64, .Height = 32, .Attachments = {FramebufferFormat::Rgba8}});
    REQUIRE(framebuffer != nullptr);
    framebuffer->ClearColor(0, {0.0f, 0.0f, 1.0f, 1.0f});
    application->PushLayer(Quasar::CreateScope<ImageLayer>(*framebuffer));
    application->PushLayer(Quasar::CreateScope<TestHelpers::ClosingLayer>(2));

    application->Run();

    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("A Depth32F attachment is read as a shadow map, which compares depths and gives 1 outside of it")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    Quasar::GraphicsContext& context = *window->GetGraphicsContext();
    const Quasar::Scope<Quasar::Framebuffer> map = CreateFramebuffer(*window, {.Width = 4, .Height = 4, .Attachments = {FramebufferFormat::Depth32F}});
    const Quasar::Scope<Quasar::Framebuffer> target = CreateFramebuffer(*window, {.Width = 1, .Height = 1, .Attachments = {FramebufferFormat::RedInteger}});
    const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::Parse("Shadow", R"(
#type vertex
#version 450 core
void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}
#type fragment
#version 450 core
layout(binding = 3) uniform sampler2DShadow u_Map;
uniform vec3 u_Coordinates;
layout(location = 0) out int o_Value;
void main()
{
    o_Value = int(round(texture(u_Map, u_Coordinates) * 100.0));
}
)");
    REQUIRE(map != nullptr);
    REQUIRE(target != nullptr);
    REQUIRE(specification.has_value());
    const Quasar::Scope<Quasar::Shader> shader = context.CreateShader(*specification);
    const Quasar::Scope<Quasar::VertexArray> vertexArray = context.CreateVertexArray();
    REQUIRE(shader != nullptr);
    REQUIRE(vertexArray != nullptr);
    map->ClearDepthStencil(0.5f);
    map->BindDepthTexture(3);
    const auto read = [&](const glm::vec3& coordinates)
    {
        shader->SetVec3("u_Coordinates", coordinates);
        target->Bind();
        shader->Bind();
        context.Draw(*vertexArray, 3);
        target->Unbind();
        return target->ReadInteger(0, 0, 0);
    };

    CHECK(read({0.5f, 0.5f, 0.4f}) == 100);
    CHECK(read({0.5f, 0.5f, 0.6f}) == 0);
    CHECK(read({1.5f, 0.5f, 0.6f}) == 100);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));

    const Quasar::Scope<Quasar::Framebuffer> stencil = CreateFramebuffer(*window, {.Width = 1, .Height = 1});
    REQUIRE(stencil != nullptr);
    target->BindDepthTexture(3);
    stencil->BindDepthTexture(3);
    const std::vector<Quasar::LogMessage> messages = Quasar::Log::GetMemorySink()->GetMessages();
    CHECK(std::ranges::count_if(messages, [](const Quasar::LogMessage& message) { return message.Text.find("no Depth32F attachment") != std::string::npos; }) ==
          2);
}

TEST_CASE("A Depth32F attachment is read as values from the closest pixel, and still as a shadow map")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    Quasar::GraphicsContext& context = *window->GetGraphicsContext();
    const Quasar::Scope<Quasar::Framebuffer> map = CreateFramebuffer(*window, {.Width = 4, .Height = 1, .Attachments = {FramebufferFormat::Depth32F}});
    const Quasar::Scope<Quasar::Framebuffer> target = CreateFramebuffer(*window, {.Width = 1, .Height = 1, .Attachments = {FramebufferFormat::RedInteger}});
    const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::Parse("Depth", R"(
#type vertex
#version 450 core
void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}
#type fragment
#version 450 core
layout(binding = 3) uniform sampler2D u_Values;
layout(binding = 4) uniform sampler2DShadow u_Map;
uniform vec2 u_Coordinates;
layout(location = 0) out int o_Value;
void main()
{
    o_Value = int(round(texture(u_Values, u_Coordinates).r * 1000.0)) + 10000 * int(round(texture(u_Map, vec3(u_Coordinates, 0.5))));
}
)");
    REQUIRE(map != nullptr);
    REQUIRE(target != nullptr);
    REQUIRE(specification.has_value());
    const Quasar::Scope<Quasar::Shader> shader = context.CreateShader(*specification);
    const Quasar::Scope<Quasar::VertexArray> vertexArray = context.CreateVertexArray();
    REQUIRE(shader != nullptr);
    REQUIRE(vertexArray != nullptr);
    map->ClearDepthStencil(0.25f);
    glEnable(GL_SCISSOR_TEST);
    glScissor(3, 0, 1, 1);
    map->ClearDepthStencil(0.75f);
    glDisable(GL_SCISSOR_TEST);
    map->BindDepthValues(3);
    map->BindDepthTexture(4);
    const auto read = [&](const glm::vec2& coordinates)
    {
        shader->SetVec2("u_Coordinates", coordinates);
        target->Bind();
        shader->Bind();
        context.Draw(*vertexArray, 3);
        target->Unbind();
        return target->ReadInteger(0, 0, 0).value_or(-1);
    };

    CHECK(read({0.1f, 0.5f}) == 250);
    CHECK(read({0.74f, 0.5f}) == 250);
    CHECK(read({0.76f, 0.5f}) == 10750);
    CHECK(read({1.5f, 0.5f}) == 10750);
    CHECK(read({-0.5f, 0.5f}) == 10250);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));

    const Quasar::Scope<Quasar::Framebuffer> stencil = CreateFramebuffer(*window, {.Width = 1, .Height = 1});
    REQUIRE(stencil != nullptr);
    target->BindDepthValues(3);
    stencil->BindDepthValues(3);
    CHECK(TestHelpers::CountErrors("no Depth32F attachment") == 2);
}

TEST_CASE("A framebuffer has attachments of one channel, of 8 bits from 0 to 1 or of a 32-bit float")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    const Quasar::Scope<Quasar::Framebuffer> framebuffer =
        CreateFramebuffer(*window, {.Width = 4, .Height = 4, .Attachments = {FramebufferFormat::Red8, FramebufferFormat::Red32F}});
    REQUIRE(framebuffer != nullptr);
    const auto readTexture = [&framebuffer](std::uint32_t attachment)
    {
        framebuffer->BindColorTexture(attachment, 5);
        glActiveTexture(GL_TEXTURE5);
        GLint texture = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
        return static_cast<GLuint>(texture);
    };

    framebuffer->ClearColor(0, {0.6f, 0.0f, 0.0f, 1.0f});
    framebuffer->ClearColor(1, {-1234.5f, 0.0f, 0.0f, 1.0f});

    GLubyte share = 0;
    glGetTextureSubImage(readTexture(0), 0, 3, 3, 0, 1, 1, 1, GL_RED, GL_UNSIGNED_BYTE, 1, &share);
    GLfloat distance = 0.0f;
    glGetTextureSubImage(readTexture(1), 0, 3, 3, 0, 1, 1, 1, GL_RED, GL_FLOAT, sizeof(distance), &distance);
    CHECK(share == 153);
    CHECK(distance == -1234.5f);
    CHECK(framebuffer->GetSpecification().Attachments == std::vector{FramebufferFormat::Red8, FramebufferFormat::Red32F});
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}
