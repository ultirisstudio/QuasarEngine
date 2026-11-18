#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"
#include "helpers/TemporaryFiles.h"

#include <quasar/core/Base.h>
#include <quasar/core/Window.h>
#include <quasar/log/Log.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Shader.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace
{
    constexpr std::string_view ShaderText = R"(
#type vertex
#version 450 core
uniform mat4 u_Transform;
uniform vec2 u_Offset;

void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = u_Transform * vec4(positions[gl_VertexID] + u_Offset, 0.0, 1.0);
}

#type fragment
#version 450 core
layout(location = 0) out int o_Value;
layout(binding = 0) uniform sampler2D u_Texture;
uniform int u_Value;
uniform bool u_Enabled;
uniform float u_Scale;
uniform vec3 u_Color;
uniform vec4 u_Tint;
uniform mat3 u_Basis;
uniform float u_Weights[2];
uniform float u_Unused;

void main()
{
    const float sum = u_Scale + u_Color.x + u_Tint.x + u_Basis[0][0] + u_Weights[0] + u_Weights[1] + texture(u_Texture, vec2(0.5)).x;
    o_Value = u_Enabled ? u_Value + int(sum) : -1;
}
)";

    Quasar::Scope<Quasar::Shader> CreateShader(const Quasar::Window& window, std::string_view text)
    {
        const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::Parse("Test", text);
        REQUIRE(specification.has_value());
        return window.GetGraphicsContext()->CreateShader(*specification);
    }

    GLuint GetBoundProgram()
    {
        GLint program = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        return static_cast<GLuint>(program);
    }

    std::ptrdiff_t CountLogs(Quasar::LogLevel level, std::string_view text)
    {
        const std::vector<Quasar::LogMessage> messages = Quasar::Log::GetMemorySink()->GetMessages();
        return std::ranges::count_if(messages, [level, text](const Quasar::LogMessage& message)
                                     { return message.Level == level && message.Text.find(text) != std::string::npos; });
    }
}

TEST_CASE("A shader is compiled from its stages, and Bind makes the draws use it")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    glUseProgram(0);

    const Quasar::Scope<Quasar::Shader> shader = CreateShader(*window, ShaderText);
    REQUIRE(shader != nullptr);
    shader->Bind();

    CHECK(shader->GetName() == "Test");
    CHECK(GetBoundProgram() != 0);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The uniforms set by name reach the shader, whether it is bound or not")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    const Quasar::Scope<Quasar::Shader> shader = CreateShader(*window, ShaderText);
    REQUIRE(shader != nullptr);
    glUseProgram(0);

    shader->SetInt("u_Value", 7);
    shader->SetInt("u_Enabled", 1);
    shader->SetInt("u_Texture", 3);
    shader->SetFloat("u_Scale", 1.5f);
    shader->SetVec2("u_Offset", {1.0f, 2.0f});
    shader->SetVec3("u_Color", {1.0f, 2.0f, 3.0f});
    shader->SetVec4("u_Tint", {1.0f, 2.0f, 3.0f, 4.0f});
    shader->SetMat3("u_Basis", glm::mat3(2.0f));
    shader->SetMat4("u_Transform", glm::mat4(3.0f));
    shader->SetFloat("u_Weights", 5.0f);
    shader->Bind();

    const GLuint program = GetBoundProgram();
    GLint value = 0;
    GLint enabled = 0;
    GLint texture = 0;
    GLfloat scale = 0.0f;
    std::array<GLfloat, 2> offset{};
    std::array<GLfloat, 3> color{};
    std::array<GLfloat, 4> tint{};
    std::array<GLfloat, 9> basis{};
    std::array<GLfloat, 16> transform{};
    GLfloat weight = 0.0f;
    glGetUniformiv(program, glGetUniformLocation(program, "u_Value"), &value);
    glGetUniformiv(program, glGetUniformLocation(program, "u_Enabled"), &enabled);
    glGetUniformiv(program, glGetUniformLocation(program, "u_Texture"), &texture);
    glGetUniformfv(program, glGetUniformLocation(program, "u_Scale"), &scale);
    glGetUniformfv(program, glGetUniformLocation(program, "u_Offset"), offset.data());
    glGetUniformfv(program, glGetUniformLocation(program, "u_Color"), color.data());
    glGetUniformfv(program, glGetUniformLocation(program, "u_Tint"), tint.data());
    glGetUniformfv(program, glGetUniformLocation(program, "u_Basis"), basis.data());
    glGetUniformfv(program, glGetUniformLocation(program, "u_Transform"), transform.data());
    glGetUniformfv(program, glGetUniformLocation(program, "u_Weights[0]"), &weight);

    CHECK(value == 7);
    CHECK(enabled == 1);
    CHECK(texture == 3);
    CHECK(scale == 1.5f);
    CHECK(offset == std::array<GLfloat, 2>{1.0f, 2.0f});
    CHECK(color == std::array<GLfloat, 3>{1.0f, 2.0f, 3.0f});
    CHECK(tint == std::array<GLfloat, 4>{1.0f, 2.0f, 3.0f, 4.0f});
    CHECK(basis == std::array<GLfloat, 9>{2.0f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 2.0f});
    CHECK(transform[0] == 3.0f);
    CHECK(transform[15] == 3.0f);
    CHECK(weight == 5.0f);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A shader draws with the values of its uniforms")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    const Quasar::Scope<Quasar::Shader> shader = CreateShader(*window, ShaderText);
    const Quasar::Scope<Quasar::Framebuffer> framebuffer =
        window->GetGraphicsContext()->CreateFramebuffer({.Width = 4, .Height = 4, .Attachments = {Quasar::FramebufferFormat::RedInteger}});
    REQUIRE(shader != nullptr);
    REQUIRE(framebuffer != nullptr);
    GLuint vertexArray = 0;
    glCreateVertexArrays(1, &vertexArray);

    shader->SetMat4("u_Transform", glm::mat4(1.0f));
    shader->SetInt("u_Value", 42);
    shader->SetInt("u_Enabled", 1);
    shader->SetFloat("u_Scale", 1.0f);
    shader->SetVec3("u_Color", {2.0f, 0.0f, 0.0f});
    shader->SetVec4("u_Tint", {3.0f, 0.0f, 0.0f, 0.0f});
    shader->SetMat3("u_Basis", glm::mat3(4.0f));
    framebuffer->ClearInteger(0, -5);
    framebuffer->Bind();
    shader->Bind();
    glBindVertexArray(vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    framebuffer->Unbind();
    glDeleteVertexArrays(1, &vertexArray);

    CHECK(framebuffer->ReadInteger(0, 0, 0) == 52);
    CHECK(framebuffer->ReadInteger(0, 3, 3) == 52);
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("A uniform of another type is refused, and an unknown uniform is reported once")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    const Quasar::Scope<Quasar::Shader> shader = CreateShader(*window, ShaderText);
    REQUIRE(shader != nullptr);
    shader->SetInt("u_Value", 7);

    shader->SetFloat("u_Value", 1.0f);
    shader->SetVec3("u_Value", {1.0f, 1.0f, 1.0f});
    for (int call = 0; call < 3; ++call)
    {
        shader->SetInt("u_Missing", call);
    }
    shader->SetFloat("u_Unused", 1.0f);
    shader->Bind();

    GLint value = 0;
    glGetUniformiv(GetBoundProgram(), glGetUniformLocation(GetBoundProgram(), "u_Value"), &value);
    CHECK(value == 7);
    CHECK(CountLogs(Quasar::LogLevel::Warning, "'u_Value'") == 1);
    CHECK(CountLogs(Quasar::LogLevel::Warning, "'u_Missing'") == 1);
    CHECK(CountLogs(Quasar::LogLevel::Warning, "'u_Unused'") == 1);
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("A stage that does not compile gives no shader, and the messages of the compiler are logged with the stage")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);

    const Quasar::Scope<Quasar::Shader> shader = CreateShader(*window, "#type vertex\n#version 450 core\nvoid main() { gl_Position = vec4(0.0); }\n"
                                                                       "#type fragment\n#version 450 core\nvoid main() { undeclared = 1.0; }\n");

    CHECK(shader == nullptr);
    CHECK(CountLogs(Quasar::LogLevel::Error, "fragment stage of the shader 'Test'") == 1);
    CHECK(CountLogs(Quasar::LogLevel::Error, "undeclared") == 1);
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Stages that do not link give no shader")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);

    const Quasar::Scope<Quasar::Shader> shader =
        CreateShader(*window, "#type vertex\n#version 450 core\nvoid main() { gl_Position = vec4(0.0); }\n"
                              "#type fragment\n#version 450 core\nin vec3 v_Missing;\nout vec4 o_Color;\nvoid main() { o_Color = vec4(v_Missing, 1.0); }\n");

    CHECK(shader == nullptr);
    CHECK(CountLogs(Quasar::LogLevel::Error, "Cannot link the shader 'Test'") == 1);
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("A shader is compiled from a file")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    const TestHelpers::TemporaryFile file("Picking.glsl", ShaderText);

    const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::FromFile(file.GetPath());
    REQUIRE(specification.has_value());
    const Quasar::Scope<Quasar::Shader> shader = window->GetGraphicsContext()->CreateShader(*specification);

    REQUIRE(shader != nullptr);
    CHECK(shader->GetName() == "Picking");
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}
