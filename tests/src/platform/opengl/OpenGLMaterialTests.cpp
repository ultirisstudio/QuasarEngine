#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Base.h>
#include <quasar/core/Window.h>
#include <quasar/io/EngineResources.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Material.h>
#include <quasar/renderer/Mesh.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/Primitives.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/Texture.h>
#include <quasar/renderer/UniformBlockLayout.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using Quasar::UniformType;

namespace
{
    // Draws a value made of one digit of each member of the block
    constexpr std::string_view ValuesShader = R"(
#type vertex
#version 450 core

void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}

#type fragment
#version 450 core
layout(std140, binding = 3) uniform Values
{
    float a;
    vec3 b;
    vec2 c;
    int d;
    vec4 e;
    mat4 f;
};
layout(location = 0) out int o_Value;

void main()
{
    o_Value = int(a) + 10 * int(b.z) + 100 * int(c.y) + 1000 * d + 10000 * int(e.w) + 100000 * int(f[3][2]);
}
)";

    constexpr std::string_view TexturesShader = R"(
#type vertex
#version 450 core

void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}

#type fragment
#version 450 core
layout(binding = 1) uniform sampler2D u_First;
layout(binding = 2) uniform sampler2D u_Second;
layout(location = 0) out int o_Value;

void main()
{
    o_Value = int(round(texture(u_First, vec2(0.5)).r * 255.0)) + 1000 * int(round(texture(u_Second, vec2(0.5)).r * 255.0));
}
)";

    Quasar::UniformBlockLayout GetValuesLayout()
    {
        return {{"a", UniformType::Float}, {"b", UniformType::Vec3}, {"c", UniformType::Vec2},
                {"d", UniformType::Int},   {"e", UniformType::Vec4}, {"f", UniformType::Mat4}};
    }

    using Color = std::array<int, 4>;

    class Scene
    {
    public:
        explicit Scene(Quasar::FramebufferFormat format) : m_Window(TestHelpers::CreateWindowWithContext())
        {
            REQUIRE(m_Window != nullptr);
            m_Framebuffer = GetContext().CreateFramebuffer({.Width = 2, .Height = 2, .Attachments = {format}});
            m_Quad = Quasar::Mesh::Create(GetContext(), Quasar::Primitives::CreateQuad(2.0f));
            m_Camera = GetContext().CreateUniformBuffer(3 * sizeof(glm::mat4) + sizeof(glm::vec4), 0);
            m_Object = GetContext().CreateUniformBuffer(sizeof(glm::mat4), 2);
            REQUIRE(m_Framebuffer != nullptr);
            REQUIRE(m_Quad != nullptr);
            REQUIRE(m_Camera != nullptr);
            REQUIRE(m_Object != nullptr);
            m_Camera->Set(glm::mat4(1.0f));
            m_Object->Set(glm::mat4(1.0f));
        }

        Quasar::GraphicsContext& GetContext() const { return *m_Window->GetGraphicsContext(); }

        Quasar::Scope<Quasar::Shader> CreateShader(const std::optional<Quasar::ShaderSpecification>& specification) const
        {
            REQUIRE(specification.has_value());
            Quasar::Scope<Quasar::Shader> shader = GetContext().CreateShader(*specification);
            REQUIRE(shader != nullptr);
            return shader;
        }

        Quasar::Scope<Quasar::Pipeline> CreatePipeline(Quasar::Shader& shader) const
        {
            Quasar::Scope<Quasar::Pipeline> pipeline = GetContext().CreatePipeline({.Shader = &shader, .DepthTest = false});
            REQUIRE(pipeline != nullptr);
            return pipeline;
        }

        Quasar::Scope<Quasar::Material> CreateMaterial(const Quasar::MaterialSpecification& specification) const
        {
            Quasar::Scope<Quasar::Material> material = Quasar::Material::Create(GetContext(), specification);
            REQUIRE(material != nullptr);
            return material;
        }

        void DrawQuad() const
        {
            m_Framebuffer->Bind();
            m_Camera->Bind();
            m_Object->Bind();
            GetContext().DrawIndexed(m_Quad->GetVertexArray());
            m_Framebuffer->Unbind();
        }

        void DrawTriangle() const
        {
            const Quasar::Scope<Quasar::VertexArray> empty = GetContext().CreateVertexArray();
            REQUIRE(empty != nullptr);
            m_Framebuffer->ClearInteger(0, -1);
            m_Framebuffer->Bind();
            GetContext().Draw(*empty, 3);
            m_Framebuffer->Unbind();
        }

        int ReadValue() const { return m_Framebuffer->ReadInteger(0, 0, 0).value_or(-2); }

        Color ReadColor(int x, int y) const
        {
            std::array<std::uint8_t, 4> pixel{};
            m_Framebuffer->Bind();
            glReadPixels(x, 1 - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
            m_Framebuffer->Unbind();
            return {pixel[0], pixel[1], pixel[2], pixel[3]};
        }

    private:
        Quasar::Scope<Quasar::Window> m_Window;
        Quasar::Scope<Quasar::Framebuffer> m_Framebuffer;
        Quasar::Scope<Quasar::Mesh> m_Quad;
        Quasar::Scope<Quasar::UniformBuffer> m_Camera;
        Quasar::Scope<Quasar::UniformBuffer> m_Object;
    };

    bool IsNear(const Color& color, const Color& expected)
    {
        return std::ranges::equal(color, expected, [](int value, int other) { return std::abs(value - other) <= 1; });
    }

    std::optional<Quasar::ShaderSpecification> ReadUnlitShader()
    {
        return Quasar::ShaderSpecification::FromFile(Quasar::GetEngineResourcesDirectory() / "shaders" / "Unlit.glsl");
    }

    GLint GetProgramValue(GLuint program, GLenum interface, const char* name, GLenum property)
    {
        const GLuint index = glGetProgramResourceIndex(program, interface, name);
        GLint value = -1;
        glGetProgramResourceiv(program, interface, index, 1, &property, 1, nullptr, &value);
        return value;
    }
}

TEST_CASE("The offsets of a uniform block layout are the ones that OpenGL gives to the GLSL block")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(Quasar::FramebufferFormat::RedInteger);
    const Quasar::Scope<Quasar::Shader> shader = scene.CreateShader(Quasar::ShaderSpecification::Parse("Values", ValuesShader));
    shader->Bind();
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    const Quasar::UniformBlockLayout layout = GetValuesLayout();

    for (std::size_t member = 0; member < layout.GetMembers().size(); ++member)
    {
        const std::string& name = layout.GetMembers()[member].Name;
        CAPTURE(name);
        CHECK(GetProgramValue(static_cast<GLuint>(program), GL_UNIFORM, name.c_str(), GL_OFFSET) == static_cast<GLint>(layout.GetOffset(member)));
    }
    CHECK(GetProgramValue(static_cast<GLuint>(program), GL_UNIFORM_BLOCK, "Values", GL_BUFFER_DATA_SIZE) == static_cast<GLint>(layout.GetSize()));
}

TEST_CASE("The values of a material reach its shader at the next Bind")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(Quasar::FramebufferFormat::RedInteger);
    const Quasar::Scope<Quasar::Shader> shader = scene.CreateShader(Quasar::ShaderSpecification::Parse("Values", ValuesShader));
    const Quasar::Scope<Quasar::Pipeline> pipeline = scene.CreatePipeline(*shader);
    const Quasar::Scope<Quasar::Material> material = scene.CreateMaterial({.Pipeline = pipeline.get(), .UniformBinding = 3, .Uniforms = GetValuesLayout()});
    glm::mat4 matrix(0.0f);
    matrix[3][2] = 6.0f;

    material->Bind();
    scene.DrawTriangle();
    CHECK(scene.ReadValue() == 0);

    material->SetFloat("a", 1.0f);
    material->SetVec3("b", {0.0f, 0.0f, 2.0f});
    material->SetVec2("c", {0.0f, 3.0f});
    material->SetInt("d", 4);
    material->SetVec4("e", {0.0f, 0.0f, 0.0f, 5.0f});
    material->SetMat4("f", matrix);
    scene.DrawTriangle();
    CHECK(scene.ReadValue() == 0);
    material->Bind();
    scene.DrawTriangle();
    CHECK(scene.ReadValue() == 654321);

    material->SetInt("d", 9);
    material->Bind();
    scene.DrawTriangle();
    CHECK(scene.ReadValue() == 659321);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("Materials that share a description keep their own values")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(Quasar::FramebufferFormat::RedInteger);
    const Quasar::Scope<Quasar::Shader> shader = scene.CreateShader(Quasar::ShaderSpecification::Parse("Values", ValuesShader));
    const Quasar::Scope<Quasar::Pipeline> pipeline = scene.CreatePipeline(*shader);
    const Quasar::MaterialSpecification specification{.Pipeline = pipeline.get(), .UniformBinding = 3, .Uniforms = GetValuesLayout()};
    const Quasar::Scope<Quasar::Material> first = scene.CreateMaterial(specification);
    const Quasar::Scope<Quasar::Material> second = scene.CreateMaterial(specification);
    first->SetInt("d", 1);
    second->SetInt("d", 2);

    first->Bind();
    scene.DrawTriangle();
    CHECK(scene.ReadValue() == 1000);
    second->Bind();
    scene.DrawTriangle();
    CHECK(scene.ReadValue() == 2000);
    first->Bind();
    scene.DrawTriangle();
    CHECK(scene.ReadValue() == 1000);
}

TEST_CASE("A value of another type or an unknown name is refused with one warning per name")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(Quasar::FramebufferFormat::RedInteger);
    const Quasar::Scope<Quasar::Shader> shader = scene.CreateShader(Quasar::ShaderSpecification::Parse("Values", ValuesShader));
    const Quasar::Scope<Quasar::Pipeline> pipeline = scene.CreatePipeline(*shader);
    const Quasar::Scope<Quasar::Material> material = scene.CreateMaterial({.Pipeline = pipeline.get(), .UniformBinding = 3, .Uniforms = GetValuesLayout()});
    material->SetInt("d", 4);

    for (int call = 0; call < 3; ++call)
    {
        material->SetFloat("d", 7.0f);
        material->SetInt("Missing", call);
        material->SetTexture("Albedo", nullptr);
    }
    material->Bind();
    scene.DrawTriangle();

    CHECK(scene.ReadValue() == 4000);
    const std::vector<Quasar::LogMessage> messages = Quasar::Log::GetMemorySink()->GetMessages();
    for (const std::string_view name : {"'d'", "'Missing'", "'Albedo'"})
    {
        CAPTURE(name);
        CHECK(std::ranges::count_if(messages, [name](const Quasar::LogMessage& message)
                                    { return message.Level == Quasar::LogLevel::Warning && message.Text.find(name) != std::string::npos; }) == 1);
    }
}

TEST_CASE("The unlit shader of the engine draws the texture of its material multiplied by its color")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(Quasar::FramebufferFormat::Rgba8);
    const Quasar::Scope<Quasar::Shader> shader = scene.CreateShader(ReadUnlitShader());
    const Quasar::Scope<Quasar::Pipeline> pipeline = scene.CreatePipeline(*shader);
    const Quasar::Scope<Quasar::Material> material =
        scene.CreateMaterial({.Pipeline = pipeline.get(), .Uniforms = {{"Color", UniformType::Vec4}}, .Textures = {{.Name = "Texture", .Slot = 0}}});
    constexpr std::array<std::uint8_t, 16> pixels{255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255};
    const Quasar::Scope<Quasar::Texture2D> texture = scene.GetContext().CreateTexture2D(
        {.Width = 2, .Height = 2, .Filter = Quasar::TextureFilter::Nearest, .Mipmaps = false}, std::as_bytes(std::span(pixels)));
    REQUIRE(texture != nullptr);

    material->SetVec4("Color", {0.5f, 1.0f, 1.0f, 1.0f});
    material->Bind();
    scene.DrawQuad();
    CHECK(IsNear(scene.ReadColor(0, 0), {128, 255, 255, 255}));

    material->SetTexture("Texture", texture.get());
    material->Bind();
    scene.DrawQuad();
    CHECK(IsNear(scene.ReadColor(0, 0), {128, 0, 0, 255}));
    CHECK(IsNear(scene.ReadColor(1, 0), {0, 255, 0, 255}));
    CHECK(IsNear(scene.ReadColor(0, 1), {0, 0, 255, 255}));
    CHECK(IsNear(scene.ReadColor(1, 1), {128, 255, 255, 255}));

    material->SetTexture("Texture", nullptr);
    material->Bind();
    scene.DrawQuad();
    CHECK(IsNear(scene.ReadColor(1, 0), {128, 255, 255, 255}));
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("Each texture of a material is bound to its slot, and a texture that is not set is white")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(Quasar::FramebufferFormat::RedInteger);
    const Quasar::Scope<Quasar::Shader> shader = scene.CreateShader(Quasar::ShaderSpecification::Parse("Textures", TexturesShader));
    const Quasar::Scope<Quasar::Pipeline> pipeline = scene.CreatePipeline(*shader);
    const Quasar::Scope<Quasar::Material> material =
        scene.CreateMaterial({.Pipeline = pipeline.get(), .Textures = {{.Name = "Second", .Slot = 2}, {.Name = "First", .Slot = 1}}});
    constexpr std::array<std::uint8_t, 4> pixel{3, 0, 0, 255};
    const Quasar::Scope<Quasar::Texture2D> texture = scene.GetContext().CreateTexture2D({.Mipmaps = false}, std::as_bytes(std::span(pixel)));
    REQUIRE(texture != nullptr);

    material->Bind();
    scene.DrawTriangle();
    CHECK(scene.ReadValue() == 255255);

    material->SetTexture("First", texture.get());
    material->Bind();
    scene.DrawTriangle();
    CHECK(scene.ReadValue() == 255003);

    material->SetTexture("First", nullptr);
    material->SetTexture("Second", texture.get());
    material->Bind();
    scene.DrawTriangle();
    CHECK(scene.ReadValue() == 3255);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A material without values or textures only binds its pipeline")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(Quasar::FramebufferFormat::RedInteger);
    const Quasar::Scope<Quasar::Shader> shader = scene.CreateShader(Quasar::ShaderSpecification::Parse("Values", ValuesShader));
    const Quasar::Scope<Quasar::Pipeline> pipeline = scene.CreatePipeline(*shader);
    const Quasar::Scope<Quasar::UniformBuffer> values = scene.GetContext().CreateUniformBuffer(GetValuesLayout().GetSize(), 3);
    REQUIRE(values != nullptr);
    values->Set(7.0f);
    values->Bind();
    glUseProgram(0);

    const Quasar::Scope<Quasar::Material> material = scene.CreateMaterial({.Pipeline = pipeline.get()});
    material->Bind();
    scene.DrawTriangle();

    CHECK(scene.ReadValue() == 7);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("No material is created from a description without a pipeline, with names used twice or a slot used twice")
{
    const Scene scene(Quasar::FramebufferFormat::RedInteger);
    const Quasar::Scope<Quasar::Shader> shader = scene.CreateShader(Quasar::ShaderSpecification::Parse("Values", ValuesShader));
    const Quasar::Scope<Quasar::Pipeline> pipeline = scene.CreatePipeline(*shader);
    GLint bindings = 0;
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &bindings);

    const std::array<Quasar::MaterialSpecification, 6> refused{{
        {.Uniforms = GetValuesLayout()},
        {.Pipeline = pipeline.get(), .Uniforms = {{"a", UniformType::Float}, {"a", UniformType::Int}}},
        {.Pipeline = pipeline.get(), .Textures = {{.Name = "Albedo", .Slot = 0}, {.Name = "Albedo", .Slot = 1}}},
        {.Pipeline = pipeline.get(), .Textures = {{.Name = "Albedo", .Slot = 2}, {.Name = "Normal", .Slot = 2}}},
        {.Pipeline = pipeline.get(), .Textures = {{.Name = "", .Slot = 0}}},
        {.Pipeline = pipeline.get(), .UniformBinding = static_cast<std::uint32_t>(bindings), .Uniforms = GetValuesLayout()},
    }};
    for (std::size_t index = 0; index < refused.size(); ++index)
    {
        const TestHelpers::ScopedLog log;
        CAPTURE(index);
        CHECK(Quasar::Material::Create(scene.GetContext(), refused.at(index)) == nullptr);
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    CHECK(glGetError() == GL_NO_ERROR);
}
