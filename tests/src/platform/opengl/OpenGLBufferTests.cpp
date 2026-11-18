#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Base.h>
#include <quasar/core/Window.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/VertexArray.h>
#include <quasar/renderer/VertexLayout.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/mat3x3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

using Quasar::VertexAttributeType;

namespace
{
    // Draws the value computed by the vertex shader from its inputs, which are given in another order than their
    // locations, from two vertex buffers
    constexpr std::string_view AttributeShader = R"(
#type vertex
#version 450 core
layout(location = 0) in vec2 a_Position;
layout(location = 1) in mat3 a_Basis;
layout(location = 5) in int a_Value;
layout(location = 0) flat out int v_Value;

void main()
{
    v_Value = a_Value * 1000 + int(a_Basis[0][0] + 10.0 * a_Basis[1][1] + 100.0 * a_Basis[2][2]);
    gl_Position = vec4(a_Position, 0.0, 1.0);
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

    constexpr std::string_view PositionShader = R"(
#type vertex
#version 450 core
layout(location = 0) in vec2 a_Position;

void main()
{
    gl_Position = vec4(a_Position, 0.0, 1.0);
}

#type fragment
#version 450 core
layout(location = 0) out int o_Value;

void main()
{
    o_Value = 1;
}
)";

    // Covers the framebuffer with one triangle, without vertex buffers
    constexpr std::string_view UniformShader = R"(
#type vertex
#version 450 core

void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}

#type fragment
#version 450 core
layout(std140, binding = 2) uniform Material
{
    vec4 u_Color;
    ivec4 u_Values;
};
layout(location = 0) out int o_Value;

void main()
{
    o_Value = u_Values.y + int(u_Color.z);
}
)";

    struct Material
    {
        glm::vec4 Color;
        glm::ivec4 Values;
    };

    struct AttributeVertex
    {
        int Value;
        glm::mat3 Basis;
    };

    Quasar::VertexLayout GetPositionLayout()
    {
        return {{0, VertexAttributeType::Vec2}};
    }

    Quasar::VertexLayout GetAttributeLayout()
    {
        return {{5, VertexAttributeType::Int}, {1, VertexAttributeType::Mat3}};
    }

    constexpr std::array<glm::vec2, 3> TrianglePositions{glm::vec2(-1.0f, -1.0f), glm::vec2(3.0f, -1.0f), glm::vec2(-1.0f, 3.0f)};
    constexpr std::array<glm::vec2, 4> QuadPositions{glm::vec2(-1.0f, -1.0f), glm::vec2(1.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec2(-1.0f, 1.0f)};

    // Draws into a framebuffer of 4x4 integers, cleared to -1
    class Scene
    {
    public:
        explicit Scene(std::string_view shaderText) : m_Window(TestHelpers::CreateWindowWithContext())
        {
            REQUIRE(m_Window != nullptr);
            const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::Parse("Test", shaderText);
            REQUIRE(specification.has_value());
            m_Shader = GetContext().CreateShader(*specification);
            m_Framebuffer = GetContext().CreateFramebuffer({.Width = 4, .Height = 4, .Attachments = {Quasar::FramebufferFormat::RedInteger}});
            REQUIRE(m_Shader != nullptr);
            REQUIRE(m_Framebuffer != nullptr);
        }

        Quasar::GraphicsContext& GetContext() const { return *m_Window->GetGraphicsContext(); }

        void Draw(const Quasar::VertexArray& vertexArray, std::uint32_t vertexCount) const
        {
            Begin();
            GetContext().Draw(vertexArray, vertexCount);
            m_Framebuffer->Unbind();
        }

        void DrawIndexed(const Quasar::VertexArray& vertexArray) const
        {
            Begin();
            GetContext().DrawIndexed(vertexArray);
            m_Framebuffer->Unbind();
        }

        // The pixels at the top left and at the bottom right
        std::pair<std::optional<int>, std::optional<int>> ReadCorners() const
        {
            return {m_Framebuffer->ReadInteger(0, 0, 0), m_Framebuffer->ReadInteger(0, 3, 3)};
        }

    private:
        void Begin() const
        {
            m_Framebuffer->ClearInteger(0, -1);
            m_Framebuffer->Bind();
            m_Shader->Bind();
        }

        Quasar::Scope<Quasar::Window> m_Window;
        Quasar::Scope<Quasar::Shader> m_Shader;
        Quasar::Scope<Quasar::Framebuffer> m_Framebuffer;
    };

    template <typename T, std::size_t Count>
    std::span<const std::byte> AsBytes(const std::array<T, Count>& values)
    {
        return std::as_bytes(std::span(values));
    }

    template <typename T>
    std::span<const std::byte> AsBytes(const T& value)
    {
        return std::as_bytes(std::span<const T, 1>(&value, 1));
    }

    std::array<AttributeVertex, 3> MakeAttributeVertices(int value)
    {
        const glm::mat3 basis(glm::vec3(1.0f, 7.0f, 7.0f), glm::vec3(7.0f, 2.0f, 7.0f), glm::vec3(7.0f, 7.0f, 3.0f));
        return {AttributeVertex{.Value = value, .Basis = basis}, AttributeVertex{.Value = value, .Basis = basis},
                AttributeVertex{.Value = value, .Basis = basis}};
    }

    std::pair<std::optional<int>, std::optional<int>> Corners(int topLeft, int bottomRight)
    {
        return {topLeft, bottomRight};
    }
}

TEST_CASE("The attributes of the vertex buffers reach the inputs of the vertex shader at the locations of their layouts")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(AttributeShader);
    const Quasar::Scope<Quasar::VertexArray> vertexArray = scene.GetContext().CreateVertexArray();
    REQUIRE(vertexArray != nullptr);

    vertexArray->AddVertexBuffer(scene.GetContext().CreateVertexBuffer(AsBytes(TrianglePositions), GetPositionLayout()));
    vertexArray->AddVertexBuffer(scene.GetContext().CreateVertexBuffer(AsBytes(MakeAttributeVertices(7)), GetAttributeLayout()));
    REQUIRE(vertexArray->GetVertexBufferCount() == 2);
    CHECK(vertexArray->GetVertexBuffer(0).GetVertexCount() == 3);
    CHECK(vertexArray->GetVertexBuffer(1).GetSize() == sizeof(AttributeVertex) * 3);
    scene.Draw(*vertexArray, 3);

    CHECK(scene.ReadCorners() == Corners(7321, 7321));
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("SetData replaces a part of a vertex buffer, and data that does not fit is refused without changing the buffer")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(AttributeShader);
    const Quasar::Scope<Quasar::VertexArray> vertexArray = scene.GetContext().CreateVertexArray();
    REQUIRE(vertexArray != nullptr);
    vertexArray->AddVertexBuffer(scene.GetContext().CreateVertexBuffer(AsBytes(TrianglePositions), GetPositionLayout()));
    vertexArray->AddVertexBuffer(scene.GetContext().CreateVertexBuffer(AsBytes(MakeAttributeVertices(7)), GetAttributeLayout()));
    REQUIRE(vertexArray->GetVertexBufferCount() == 2);
    Quasar::VertexBuffer& buffer = vertexArray->GetVertexBuffer(1);

    constexpr int value = 4;
    for (std::size_t vertex = 0; vertex < 3; ++vertex)
    {
        buffer.SetData(AsBytes(value), vertex * sizeof(AttributeVertex) + offsetof(AttributeVertex, Value));
    }
    scene.Draw(*vertexArray, 3);
    CHECK(scene.ReadCorners() == Corners(4321, 4321));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));

    buffer.SetData(AsBytes(MakeAttributeVertices(9)), 1);
    buffer.SetData(AsBytes(value), buffer.GetSize() - 3);
    buffer.SetData(AsBytes(value), std::numeric_limits<std::size_t>::max());
    scene.Draw(*vertexArray, 3);

    CHECK(scene.ReadCorners() == Corners(4321, 4321));
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("An index buffer draws the triangles of its indices, and SetData replaces some of them")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(PositionShader);
    const Quasar::Scope<Quasar::VertexArray> vertexArray = scene.GetContext().CreateVertexArray();
    REQUIRE(vertexArray != nullptr);
    vertexArray->AddVertexBuffer(scene.GetContext().CreateVertexBuffer(AsBytes(QuadPositions), GetPositionLayout()));
    constexpr std::array<std::uint32_t, 6> indices{0, 1, 2, 2, 3, 0};
    vertexArray->SetIndexBuffer(scene.GetContext().CreateIndexBuffer(indices));
    Quasar::IndexBuffer* indexBuffer = vertexArray->GetIndexBuffer();
    REQUIRE(indexBuffer != nullptr);
    CHECK(indexBuffer->GetCount() == 6);

    scene.DrawIndexed(*vertexArray);
    CHECK(scene.ReadCorners() == Corners(1, 1));

    constexpr std::array<std::uint32_t, 3> lowerTriangle{0, 1, 2};
    indexBuffer->SetData(lowerTriangle, 3);
    scene.DrawIndexed(*vertexArray);
    CHECK(scene.ReadCorners() == Corners(-1, 1));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));

    constexpr std::array<std::uint32_t, 3> upperTriangle{2, 3, 0};
    indexBuffer->SetData(upperTriangle, 4);
    scene.DrawIndexed(*vertexArray);
    CHECK(scene.ReadCorners() == Corners(-1, 1));
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("A draw of more vertices than a vertex buffer holds, or an indexed draw without an index buffer, is refused")
{
    const Scene scene(PositionShader);
    const Quasar::Scope<Quasar::VertexArray> vertexArray = scene.GetContext().CreateVertexArray();
    REQUIRE(vertexArray != nullptr);
    vertexArray->AddVertexBuffer(scene.GetContext().CreateVertexBuffer(AsBytes(TrianglePositions), GetPositionLayout()));
    {
        const TestHelpers::ScopedLog log;
        scene.Draw(*vertexArray, 4);
        CHECK(scene.ReadCorners() == Corners(-1, -1));
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    {
        const TestHelpers::ScopedLog log;
        scene.DrawIndexed(*vertexArray);
        CHECK(scene.ReadCorners() == Corners(-1, -1));
        CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    }
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("A vertex array refuses null buffers and a vertex buffer whose locations another vertex buffer uses")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(PositionShader);
    const Quasar::Scope<Quasar::VertexArray> vertexArray = scene.GetContext().CreateVertexArray();
    REQUIRE(vertexArray != nullptr);
    vertexArray->AddVertexBuffer(scene.GetContext().CreateVertexBuffer(AsBytes(TrianglePositions), GetPositionLayout()));
    vertexArray->AddVertexBuffer(scene.GetContext().CreateVertexBuffer(AsBytes(MakeAttributeVertices(1)), GetAttributeLayout()));
    REQUIRE(vertexArray->GetVertexBufferCount() == 2);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));

    vertexArray->AddVertexBuffer(scene.GetContext().CreateVertexBuffer(AsBytes(TrianglePositions), GetPositionLayout()));
    vertexArray->AddVertexBuffer(scene.GetContext().CreateVertexBuffer(AsBytes(TrianglePositions), Quasar::VertexLayout{{3, VertexAttributeType::Vec2}}));
    vertexArray->AddVertexBuffer(nullptr);
    vertexArray->SetIndexBuffer(nullptr);

    CHECK(vertexArray->GetVertexBufferCount() == 2);
    CHECK(vertexArray->GetIndexBuffer() == nullptr);
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    scene.Draw(*vertexArray, 3);
    CHECK(scene.ReadCorners() == Corners(1, 1));
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("A uniform buffer starts with zeros, and Bind makes the draws read its uniform block")
{
    const TestHelpers::ScopedLog log;
    const Scene scene(UniformShader);
    const Quasar::Scope<Quasar::VertexArray> vertexArray = scene.GetContext().CreateVertexArray();
    const Quasar::Scope<Quasar::UniformBuffer> material = scene.GetContext().CreateUniformBuffer(sizeof(Material), 2);
    const Quasar::Scope<Quasar::UniformBuffer> other = scene.GetContext().CreateUniformBuffer(sizeof(Material), 2);
    REQUIRE(vertexArray != nullptr);
    REQUIRE(material != nullptr);
    REQUIRE(other != nullptr);
    CHECK(material->GetSize() == sizeof(Material));
    CHECK(material->GetBinding() == 2);

    material->Bind();
    scene.Draw(*vertexArray, 3);
    CHECK(scene.ReadCorners() == Corners(0, 0));

    material->Set(Material{.Color = {0.0f, 0.0f, 5.0f, 0.0f}, .Values = {0, 40, 0, 0}});
    other->Set(Material{.Color = {0.0f, 0.0f, 0.0f, 0.0f}, .Values = {0, 80, 0, 0}});
    scene.Draw(*vertexArray, 3);
    CHECK(scene.ReadCorners() == Corners(45, 45));

    other->Bind();
    scene.Draw(*vertexArray, 3);
    CHECK(scene.ReadCorners() == Corners(80, 80));

    material->Bind();
    material->SetData(AsBytes(glm::ivec4(0, 60, 0, 0)), offsetof(Material, Values));
    scene.Draw(*vertexArray, 3);
    CHECK(scene.ReadCorners() == Corners(65, 65));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));

    material->SetData(AsBytes(glm::ivec4(0, 90, 0, 0)), offsetof(Material, Values) + 4);
    material->Set(std::array<Material, 2>{});
    scene.Draw(*vertexArray, 3);
    CHECK(scene.ReadCorners() == Corners(65, 65));
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Buffers that the graphics API cannot create are refused")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    Quasar::GraphicsContext& context = *window->GetGraphicsContext();
    GLint bindings = 0;
    GLint blockSize = 0;
    glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &bindings);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &blockSize);

    CHECK(context.CreateUniformBuffer(16, static_cast<std::uint32_t>(bindings) - 1) != nullptr);
    CHECK(context.CreateUniformBuffer(static_cast<std::size_t>(blockSize), 0) != nullptr);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));

    CHECK(context.CreateUniformBuffer(16, static_cast<std::uint32_t>(bindings)) == nullptr);
    CHECK(context.CreateUniformBuffer(static_cast<std::size_t>(blockSize) + 1, 0) == nullptr);
    CHECK(context.CreateUniformBuffer(0, 0) == nullptr);
    CHECK(context.CreateVertexBuffer({}, GetPositionLayout()) == nullptr);
    CHECK(context.CreateVertexBuffer(AsBytes(TrianglePositions), Quasar::VertexLayout{{0, VertexAttributeType::Vec4}}) == nullptr);
    CHECK(context.CreateIndexBuffer({}) == nullptr);
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
    CHECK(glGetError() == GL_NO_ERROR);
}
