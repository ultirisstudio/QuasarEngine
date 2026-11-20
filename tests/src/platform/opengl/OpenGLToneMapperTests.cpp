#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"
#include "helpers/RenderHelpers.h"

#include <quasar/core/Base.h>
#include <quasar/core/Window.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/ToneMapper.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <array>

using TestHelpers::Color;
using TestHelpers::IsNear;
using TestHelpers::ReadColor;
using TestHelpers::ReadWindowPixel;

namespace
{
    class Fixture
    {
    public:
        Fixture()
        {
            m_Window = TestHelpers::CreateWindowWithContext();
            REQUIRE(m_Window != nullptr);
            m_ToneMapper = Quasar::ToneMapper::Create(GetContext());
            m_Source = GetContext().CreateFramebuffer(
                {.Width = 4, .Height = 4, .Attachments = {Quasar::FramebufferFormat::Rgba16F, Quasar::FramebufferFormat::Rgba16F}});
            m_Target = GetContext().CreateFramebuffer({.Width = 4, .Height = 4});
            REQUIRE(m_ToneMapper != nullptr);
            REQUIRE(m_Source != nullptr);
            REQUIRE(m_Target != nullptr);
        }

        Quasar::GraphicsContext& GetContext() const { return *m_Window->GetGraphicsContext(); }

        Color Apply(const glm::vec4& linear, float exposure, Quasar::Tonemapping tonemapping) const
        {
            m_Source->ClearColor(0, linear);
            m_Source->ClearColor(1, glm::vec4(0.0f));
            m_ToneMapper->Apply(*m_Source, m_Target.get(), exposure, tonemapping);
            return ReadColor(*m_Target, 1, 1);
        }

        Quasar::ToneMapper& GetToneMapper() const { return *m_ToneMapper; }
        Quasar::Framebuffer& GetSource() const { return *m_Source; }
        Quasar::Framebuffer& GetTarget() const { return *m_Target; }

    private:
        Quasar::Scope<Quasar::Window> m_Window;
        Quasar::Scope<Quasar::ToneMapper> m_ToneMapper;
        Quasar::Scope<Quasar::Framebuffer> m_Source;
        Quasar::Scope<Quasar::Framebuffer> m_Target;
    };
}

TEST_CASE("The tone mapper writes the first color attachment of its source in sRGB, after the exposure and the tonemapping")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;

    CHECK(IsNear(fixture.Apply({0.5f, 2.0f, -1.0f, 0.25f}, 1.0f, Quasar::Tonemapping::None), {188, 255, 0, 255}));
    CHECK(IsNear(fixture.Apply({0.25f, 0.0f, 0.0f, 1.0f}, 2.0f, Quasar::Tonemapping::None), {188, 0, 0, 255}));
    CHECK(IsNear(fixture.Apply({1.0f, 2.0f, 0.0f, 1.0f}, 1.0f, Quasar::Tonemapping::Reinhard), {188, 213, 0, 255}));
    CHECK(IsNear(fixture.Apply({1.0f, 2.0f, 0.0f, 1.0f}, 1.0f, Quasar::Tonemapping::Aces), {232, 245, 0, 255}));
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The tone mapper draws into the window without a target, and leaves the window bound")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    const auto isWindowBound = [&fixture]
    {
        GLint framebuffer = -1;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
        std::array<GLint, 4> viewport{};
        glGetIntegerv(GL_VIEWPORT, viewport.data());
        const glm::uvec2 size = fixture.GetContext().GetWindowSize();
        return framebuffer == 0 && viewport[2] == static_cast<GLint>(size.x) && viewport[3] == static_cast<GLint>(size.y);
    };
    fixture.GetContext().ClearWindow({1.0f, 0.0f, 1.0f, 1.0f});
    fixture.GetSource().ClearColor(0, {0.0f, 0.5f, 0.0f, 1.0f});

    fixture.GetTarget().Bind();
    fixture.GetToneMapper().Apply(fixture.GetSource(), &fixture.GetTarget(), 1.0f, Quasar::Tonemapping::None);
    CHECK(isWindowBound());

    fixture.GetTarget().Bind();
    fixture.GetToneMapper().Apply(fixture.GetSource(), nullptr, 1.0f, Quasar::Tonemapping::None);
    CHECK(isWindowBound());

    CHECK(IsNear(ReadWindowPixel(1, 1), {0, 188, 0, 255}));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The tone mapper resolves a multisampled target, so its image can be read")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;
    const Quasar::Scope<Quasar::Framebuffer> multisampled =
        fixture.GetContext().CreateFramebuffer({.Width = 4, .Height = 4, .Attachments = {Quasar::FramebufferFormat::Rgba16F}, .Samples = 4});
    REQUIRE(multisampled != nullptr);
    fixture.GetSource().ClearColor(0, {0.5f, 0.0f, 0.0f, 1.0f});

    fixture.GetToneMapper().Apply(fixture.GetSource(), multisampled.get(), 1.0f, Quasar::Tonemapping::None);
    fixture.GetToneMapper().Apply(*multisampled, &fixture.GetTarget(), 1.0f, Quasar::Tonemapping::None);

    CHECK(IsNear(ReadColor(fixture.GetTarget(), 1, 1), {223, 0, 0, 255}));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}
