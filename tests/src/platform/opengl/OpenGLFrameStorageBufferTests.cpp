#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Base.h>
#include <quasar/core/Window.h>
#include <quasar/renderer/Buffer.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Pipeline.h>
#include <quasar/renderer/RenderStatistics.h>
#include <quasar/renderer/Shader.h>
#include <quasar/renderer/VertexArray.h>

#include <doctest/doctest.h>

#include <glad/gl.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace
{
    constexpr std::uint32_t Binding = 4;

    constexpr std::string_view ElementShader = R"(
#type vertex
#version 450 core

void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}

#type fragment
#version 450 core
struct Element
{
    int Value;
    int Other;
};
layout(std430, binding = 4) readonly buffer Elements
{
    Element u_Elements[];
};
uniform int u_Index;
layout(location = 0) out int o_Value;

void main()
{
    o_Value = u_Elements[u_Index].Value + u_Elements[u_Index].Other;
}
)";

    struct ElementData
    {
        std::int32_t Value = 0;
        std::int32_t Other = 0;
    };

    class Fixture
    {
    public:
        Fixture() : m_Window(TestHelpers::CreateWindowWithContext())
        {
            REQUIRE(m_Window != nullptr);
            const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::Parse("Element", ElementShader);
            REQUIRE(specification.has_value());
            m_Shader = GetContext().CreateShader(*specification);
            REQUIRE(m_Shader != nullptr);
            m_Pipeline = GetContext().CreatePipeline({.Shader = m_Shader.get(), .DepthTest = false});
            m_VertexArray = GetContext().CreateVertexArray();
            m_Buffer = GetContext().CreateFrameStorageBuffer(sizeof(ElementData), Binding);
            m_Target = GetContext().CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {Quasar::FramebufferFormat::RedInteger}});
            REQUIRE(m_Pipeline != nullptr);
            REQUIRE(m_VertexArray != nullptr);
            REQUIRE(m_Buffer != nullptr);
            REQUIRE(m_Target != nullptr);
        }

        Quasar::GraphicsContext& GetContext() const { return *m_Window->GetGraphicsContext(); }
        Quasar::FrameStorageBuffer& GetBuffer() const { return *m_Buffer; }

        std::optional<std::uint32_t> Add(std::int32_t value) const { return m_Buffer->Add(ElementData{.Value = value, .Other = 1}); }

        std::optional<int> DrawAndRead(std::uint32_t index) const
        {
            m_Target->ClearInteger(0, -1);
            m_Target->Bind();
            m_Pipeline->Bind();
            m_Shader->SetInt("u_Index", static_cast<int>(index));
            m_Buffer->Bind();
            GetContext().Draw(*m_VertexArray, 3);
            m_Target->Unbind();
            return m_Target->ReadInteger(0, 0, 0);
        }

    private:
        Quasar::Scope<Quasar::Window> m_Window;
        Quasar::Scope<Quasar::Shader> m_Shader;
        Quasar::Scope<Quasar::Pipeline> m_Pipeline;
        Quasar::Scope<Quasar::VertexArray> m_VertexArray;
        Quasar::Scope<Quasar::FrameStorageBuffer> m_Buffer;
        Quasar::Scope<Quasar::Framebuffer> m_Target;
    };
}

TEST_CASE("The elements of a frame storage buffer follow each other, and the draws read them by index")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;

    CHECK(fixture.Add(10) == 0U);
    CHECK(fixture.Add(20) == 1U);
    CHECK(fixture.Add(30) == 2U);

    CHECK(fixture.GetBuffer().GetCount() == 3);
    CHECK(fixture.GetBuffer().GetElementSize() == sizeof(ElementData));
    CHECK(fixture.GetBuffer().GetBinding() == Binding);
    CHECK(fixture.DrawAndRead(1) == 21);
    CHECK(fixture.DrawAndRead(0) == 11);
    CHECK(fixture.DrawAndRead(2) == 31);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A frame storage buffer refuses elements of another size, empty elements and bindings that OpenGL does not have")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;

    constexpr std::array<std::byte, 4> small{};
    CHECK_FALSE(fixture.GetBuffer().AddData(small).has_value());
    CHECK(TestHelpers::CountErrors("elements of 8 bytes") == 1);
    CHECK(fixture.GetBuffer().GetCount() == 0);
    CHECK(fixture.GetContext().CreateFrameStorageBuffer(0, Binding) == nullptr);
    CHECK(fixture.GetContext().CreateFrameStorageBuffer(16, 100000) == nullptr);
}

TEST_CASE("A frame storage buffer keeps the elements of a frame when it grows, and starts again from the element 0 at the next frame")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    for (std::int32_t value = 0; value < 200; ++value)
    {
        REQUIRE(fixture.Add(value * 10) == static_cast<std::uint32_t>(value));
    }

    for (const std::uint32_t index : {0U, 63U, 64U, 128U, 199U})
    {
        CAPTURE(index);
        CHECK(fixture.DrawAndRead(index) == (static_cast<int>(index) * 10) + 1);
    }

    fixture.GetContext().EndFrame();
    CHECK(fixture.GetBuffer().GetCount() == 0);
    CHECK(fixture.Add(7) == 0U);
    CHECK(fixture.DrawAndRead(0) == 8);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("Three frames in a row bind their elements from three different places, which the fourth frame uses again")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    std::vector<GLint64> starts;

    for (int frame = 0; frame < 4; ++frame)
    {
        REQUIRE(fixture.Add(frame) == 0U);
        fixture.GetBuffer().Bind();
        GLint64 start = -1;
        glGetInteger64i_v(GL_SHADER_STORAGE_BUFFER_START, Binding, &start);
        starts.push_back(start);
        fixture.GetContext().EndFrame();
    }

    CHECK(starts[0] != starts[1]);
    CHECK(starts[1] != starts[2]);
    CHECK(starts[0] != starts[2]);
    CHECK(starts[3] == starts[0]);
}

TEST_CASE("The elements added to a frame storage buffer are counted as uploaded bytes, and the buffer is bound once per frame")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    fixture.GetContext().EndFrame();

    REQUIRE(fixture.Add(1).has_value());
    REQUIRE(fixture.Add(2).has_value());
    fixture.GetBuffer().Bind();
    fixture.GetBuffer().Bind();
    fixture.GetContext().EndFrame();

    const Quasar::RenderStatistics& statistics = fixture.GetContext().GetFrameStatistics();
    CHECK(statistics.Uploads == 0);
    CHECK(statistics.UploadedBytes == 2 * sizeof(ElementData));
    CHECK(statistics.StorageBufferBinds == 1);
}
