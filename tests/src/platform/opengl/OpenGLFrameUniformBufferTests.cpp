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
#include <glm/vec4.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace
{
    constexpr std::uint32_t Binding = 6;

    constexpr std::string_view ValueShader = R"(
#type vertex
#version 450 core

void main()
{
    const vec2 positions[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}

#type fragment
#version 450 core
layout(std140, binding = 6) uniform Part
{
    ivec4 u_Value;
};
layout(location = 0) out int o_Value;

void main()
{
    o_Value = u_Value.x;
}
)";

    struct PartData
    {
        std::int32_t Value = 0;
        std::array<std::int32_t, 4> Padding{};
    };

    class Fixture
    {
    public:
        Fixture() : m_Window(TestHelpers::CreateWindowWithContext())
        {
            REQUIRE(m_Window != nullptr);
            const std::optional<Quasar::ShaderSpecification> specification = Quasar::ShaderSpecification::Parse("Value", ValueShader);
            REQUIRE(specification.has_value());
            m_Shader = GetContext().CreateShader(*specification);
            REQUIRE(m_Shader != nullptr);
            m_Pipeline = GetContext().CreatePipeline({.Shader = m_Shader.get(), .DepthTest = false});
            m_VertexArray = GetContext().CreateVertexArray();
            m_Buffer = GetContext().CreateFrameUniformBuffer(sizeof(PartData), Binding);
            REQUIRE(m_Pipeline != nullptr);
            REQUIRE(m_VertexArray != nullptr);
            REQUIRE(m_Buffer != nullptr);
        }

        Quasar::GraphicsContext& GetContext() const { return *m_Window->GetGraphicsContext(); }
        Quasar::FrameUniformBuffer& GetBuffer() const { return *m_Buffer; }

        std::optional<std::uint32_t> Add(std::int32_t value) const { return m_Buffer->Add(PartData{.Value = value}); }

        Quasar::Scope<Quasar::Framebuffer> CreateTarget() const
        {
            Quasar::Scope<Quasar::Framebuffer> target =
                GetContext().CreateFramebuffer({.Width = 1, .Height = 1, .Attachments = {Quasar::FramebufferFormat::RedInteger}});
            REQUIRE(target != nullptr);
            target->ClearInteger(0, -1);
            return target;
        }

        void Draw(std::uint32_t part, Quasar::Framebuffer& target) const
        {
            target.Bind();
            m_Pipeline->Bind();
            m_Buffer->Bind(part);
            GetContext().Draw(*m_VertexArray, 3);
            target.Unbind();
        }

        std::optional<int> DrawAndRead(std::uint32_t part) const
        {
            const Quasar::Scope<Quasar::Framebuffer> target = CreateTarget();
            Draw(part, *target);
            return target->ReadInteger(0, 0, 0);
        }

    private:
        Quasar::Scope<Quasar::Window> m_Window;
        Quasar::Scope<Quasar::Shader> m_Shader;
        Quasar::Scope<Quasar::Pipeline> m_Pipeline;
        Quasar::Scope<Quasar::VertexArray> m_VertexArray;
        Quasar::Scope<Quasar::FrameUniformBuffer> m_Buffer;
    };
}

TEST_CASE("Each part of a frame uniform buffer keeps its own data, which the draws read from the bound part")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;

    CHECK(fixture.Add(10) == 0U);
    CHECK(fixture.Add(20) == 1U);
    CHECK(fixture.Add(30) == 2U);

    CHECK(fixture.GetBuffer().GetPartCount() == 3);
    CHECK(fixture.GetBuffer().GetPartSize() == sizeof(PartData));
    CHECK(fixture.GetBuffer().GetBinding() == Binding);
    CHECK(fixture.DrawAndRead(1) == 20);
    CHECK(fixture.DrawAndRead(0) == 10);
    CHECK(fixture.DrawAndRead(2) == 30);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A frame uniform buffer refuses parts of another size and parts that the current frame did not add")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    REQUIRE(fixture.Add(10) == 0U);

    constexpr std::array<std::byte, 4> small{};
    CHECK_FALSE(fixture.GetBuffer().AddData(small).has_value());
    CHECK(TestHelpers::CountErrors("parts of 20 bytes") == 1);
    fixture.GetBuffer().Bind(1);
    CHECK(TestHelpers::CountErrors("the current frame added 1 parts") == 1);
    CHECK(fixture.GetBuffer().GetPartCount() == 1);

    CHECK(fixture.GetContext().CreateFrameUniformBuffer(0, Binding) == nullptr);
    CHECK(fixture.GetContext().CreateFrameUniformBuffer(16, 100000) == nullptr);
    CHECK(fixture.GetContext().CreateFrameUniformBuffer(100000000, Binding) == nullptr);
}

TEST_CASE("A frame uniform buffer keeps the parts of a frame when it grows, and starts again from the part 0 at the next frame")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    for (std::int32_t value = 0; value < 50; ++value)
    {
        REQUIRE(fixture.Add(value * 10) == static_cast<std::uint32_t>(value));
    }
    const Quasar::Scope<Quasar::Framebuffer> beforeGrowing = fixture.CreateTarget();
    fixture.Draw(49, *beforeGrowing);
    for (std::int32_t value = 50; value < 200; ++value)
    {
        REQUIRE(fixture.Add(value * 10) == static_cast<std::uint32_t>(value));
    }

    CHECK(beforeGrowing->ReadInteger(0, 0, 0) == 490);
    for (const std::uint32_t part : {0U, 63U, 64U, 128U, 199U})
    {
        CAPTURE(part);
        CHECK(fixture.DrawAndRead(part) == static_cast<int>(part) * 10);
    }

    fixture.GetContext().EndFrame();
    CHECK(fixture.GetBuffer().GetPartCount() == 0);
    CHECK(fixture.Add(7) == 0U);
    CHECK(fixture.DrawAndRead(0) == 7);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("A frame does not write over the parts that the draws of the previous frames still read")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    std::vector<Quasar::Scope<Quasar::Framebuffer>> targets;

    for (std::int32_t frame = 0; frame < 5; ++frame)
    {
        REQUIRE(fixture.Add(100 + frame) == 0U);
        targets.push_back(fixture.CreateTarget());
        fixture.Draw(0, *targets.back());
        fixture.GetContext().EndFrame();
    }

    for (std::size_t frame = 0; frame < targets.size(); ++frame)
    {
        CAPTURE(frame);
        CHECK(targets[frame]->ReadInteger(0, 0, 0) == 100 + static_cast<int>(frame));
    }
    CHECK(glGetError() == GL_NO_ERROR);
}

TEST_CASE("Three frames in a row write their parts into three different places, which the fourth frame uses again")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    std::vector<GLint64> starts;

    for (int frame = 0; frame < 4; ++frame)
    {
        REQUIRE(fixture.Add(frame) == 0U);
        fixture.GetBuffer().Bind(0);
        GLint64 start = -1;
        glGetInteger64i_v(GL_UNIFORM_BUFFER_START, Binding, &start);
        starts.push_back(start);
        fixture.GetContext().EndFrame();
    }

    CHECK(starts[0] != starts[1]);
    CHECK(starts[1] != starts[2]);
    CHECK(starts[0] != starts[2]);
    CHECK(starts[3] == starts[0]);
}

TEST_CASE("The parts added to a frame uniform buffer are counted as uploaded bytes, without uploads to the driver")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    fixture.GetContext().EndFrame();

    REQUIRE(fixture.Add(1).has_value());
    REQUIRE(fixture.Add(2).has_value());
    fixture.GetBuffer().Bind(1);
    fixture.GetBuffer().Bind(1);
    fixture.GetBuffer().Bind(0);
    fixture.GetContext().EndFrame();

    const Quasar::RenderStatistics& statistics = fixture.GetContext().GetFrameStatistics();
    CHECK(statistics.Uploads == 0);
    CHECK(statistics.UploadedBytes == 2 * sizeof(PartData));
    CHECK(statistics.UniformBufferBinds == 2);
}
