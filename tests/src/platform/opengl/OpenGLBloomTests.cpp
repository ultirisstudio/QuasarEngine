#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Base.h>
#include <quasar/core/Window.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/renderer/Bloom.h>
#include <quasar/renderer/Camera.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/PostProcessing.h>

#include <doctest/doctest.h>

#include <glad/gl.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

using Quasar::BloomSettings;

namespace
{
    class Fixture
    {
    public:
        Fixture()
        {
            m_Window = TestHelpers::CreateWindowWithContext();
            REQUIRE(m_Window != nullptr);
            m_Bloom = Quasar::Bloom::Create(GetContext());
            REQUIRE(m_Bloom != nullptr);
        }

        Quasar::GraphicsContext& GetContext() const { return *m_Window->GetGraphicsContext(); }
        Quasar::Bloom& GetBloom() const { return *m_Bloom; }

        void Resize(std::uint32_t width, std::uint32_t height)
        {
            m_Source = GetContext().CreateFramebuffer({.Width = width, .Height = height, .Attachments = {Quasar::FramebufferFormat::Rgba16F}});
            m_Target = GetContext().CreateFramebuffer({.Width = width, .Height = height, .Attachments = {Quasar::FramebufferFormat::Rgba16F}});
            REQUIRE(m_Source != nullptr);
            REQUIRE(m_Target != nullptr);
            m_Source->ClearColor(0, {0.0f, 0.0f, 0.0f, 1.0f});
        }

        void Fill(float value, float alpha = 1.0f) const { m_Source->ClearColor(0, {value, value, value, alpha}); }

        void FillSquare(glm::ivec2 corner, int size, float value) const
        {
            glEnable(GL_SCISSOR_TEST);
            glScissor(corner.x, corner.y, size, size);
            m_Source->ClearColor(0, {value, value, value, 1.0f});
            glDisable(GL_SCISSOR_TEST);
        }

        void Apply(const BloomSettings& settings) const
        {
            m_Bloom->SetSettings(settings);
            const Quasar::FramebufferSpecification& specification = m_Source->GetSpecification();
            const Quasar::PostProcessInput input{
                .Source = m_Source.get(), .Scene = m_Source.get(), .View = &m_Camera, .Size = {specification.Width, specification.Height}};
            m_Bloom->Apply(input, m_Target.get());
        }

        float Read(int x, int y, std::size_t channel = 0) const
        {
            std::array<float, 4> pixel{};
            m_Target->Bind();
            glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, pixel.data());
            m_Target->Unbind();
            return pixel.at(channel);
        }

    private:
        Quasar::Scope<Quasar::Window> m_Window;
        Quasar::Scope<Quasar::Bloom> m_Bloom;
        Quasar::Scope<Quasar::Framebuffer> m_Source;
        Quasar::Scope<Quasar::Framebuffer> m_Target;
        Quasar::Camera m_Camera;
    };
}

TEST_CASE("A bloom is an enabled effect that runs before the tone mapping, with the default settings")
{
    const TestHelpers::ScopedLog log;
    const Fixture fixture;

    CHECK(fixture.GetBloom().GetName() == "Bloom");
    CHECK(fixture.GetBloom().GetStage() == Quasar::PostProcessStage::BeforeToneMapping);
    CHECK(fixture.GetBloom().IsEnabled());
    CHECK(fixture.GetBloom().GetSettings().Threshold == 1.0f);
    CHECK(fixture.GetBloom().GetSettings().Intensity == 0.1f);
    CHECK(fixture.GetBloom().GetLevelCount() == 0);
}

TEST_CASE("A bloom keeps the brightness of a uniform image and adds its intensity, at any size")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    const BloomSettings everything{.Threshold = 0.0f, .SoftKnee = 0.0f, .Intensity = 0.5f};

    for (const auto& [width, height, levels] :
         {std::array<std::uint32_t, 3>{32, 32, 5}, std::array<std::uint32_t, 3>{256, 256, 6}, std::array<std::uint32_t, 3>{7, 5, 2},
          std::array<std::uint32_t, 3>{1, 16, 4}, std::array<std::uint32_t, 3>{16, 1, 4}, std::array<std::uint32_t, 3>{1, 1, 1}})
    {
        CAPTURE(width);
        CAPTURE(height);
        fixture.Resize(width, height);
        fixture.Fill(4.0f);
        fixture.Apply(everything);
        CHECK(fixture.GetBloom().GetLevelCount() == levels);
        CHECK(fixture.Read(0, 0) == doctest::Approx(6.0).epsilon(0.002));
        CHECK(fixture.Read(static_cast<int>(width) - 1, static_cast<int>(height) - 1) == doctest::Approx(6.0).epsilon(0.002));
    }
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("Only the parts above the threshold glow, faded in below it by the soft knee, and an intensity of 0 adds nothing")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    fixture.Resize(16, 16);

    fixture.Fill(0.4f);
    fixture.Apply({.Threshold = 1.0f, .SoftKnee = 0.5f, .Intensity = 1.0f});
    CHECK(fixture.Read(8, 8) == doctest::Approx(0.4).epsilon(0.002));

    fixture.Fill(1.0f);
    fixture.Apply({.Threshold = 1.0f, .SoftKnee = 0.0f, .Intensity = 1.0f});
    CHECK(fixture.Read(8, 8) == doctest::Approx(1.0).epsilon(0.002));
    fixture.Apply({.Threshold = 1.0f, .SoftKnee = 0.5f, .Intensity = 1.0f});
    CHECK(fixture.Read(8, 8) == doctest::Approx(1.125).epsilon(0.002));
    fixture.Apply({.Threshold = 0.5f, .SoftKnee = 0.0f, .Intensity = 1.0f});
    CHECK(fixture.Read(8, 8) == doctest::Approx(1.5).epsilon(0.002));
    fixture.Apply({.Threshold = -1.0f, .SoftKnee = 2.0f, .Intensity = 1.0f});
    CHECK(fixture.Read(8, 8) == doctest::Approx(2.0).epsilon(0.002));
    fixture.Apply({.Threshold = 1.0f, .SoftKnee = 2.0f, .Intensity = 1.0f});
    CHECK(fixture.Read(8, 8) == doctest::Approx(1.25).epsilon(0.002));

    fixture.Fill(1.2f, 0.25f);
    fixture.Apply({.Threshold = 1.0f, .SoftKnee = 0.5f, .Intensity = 1.0f});
    CHECK(fixture.Read(8, 8) == doctest::Approx(1.445).epsilon(0.002));
    CHECK(fixture.Read(8, 8, 3) == doctest::Approx(0.25).epsilon(0.002));

    fixture.Fill(-1.0f);
    fixture.Apply({.Threshold = 0.0f, .SoftKnee = 0.0f, .Intensity = 1.0f});
    CHECK(fixture.Read(8, 8) == doctest::Approx(-1.0).epsilon(0.002));

    fixture.Fill(8.0f);
    fixture.Apply({.Threshold = 0.0f, .Intensity = 0.0f});
    CHECK(fixture.Read(8, 8) == doctest::Approx(8.0).epsilon(0.002));
    fixture.Apply({.Threshold = 0.0f, .Intensity = -1.0f});
    CHECK(fixture.Read(8, 8) == doctest::Approx(8.0).epsilon(0.002));
}

TEST_CASE("A bright spot glows over its surroundings, further with a larger radius")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    fixture.Resize(64, 64);
    fixture.FillSquare({30, 30}, 4, 50.0f);
    const auto glowAt = [&fixture](float radius, int x)
    {
        fixture.Apply({.Intensity = 1.0f, .Radius = radius});
        return fixture.Read(x, 31);
    };

    CHECK(glowAt(1.0f, 31) > 50.0f);
    const float near = glowAt(1.0f, 28);
    const float far = glowAt(1.0f, 12);
    CHECK(near > 0.5f);
    CHECK(far > 0.0f);
    CHECK(far < near / 4.0f);
    CHECK(fixture.Read(0, 0) < far);
    fixture.Apply({.Intensity = 1.0f});
    for (const int distance : {3, 6, 12})
    {
        CAPTURE(distance);
        const float left = fixture.Read(30 - distance, 31);
        CHECK(std::abs(fixture.Read(33 + distance, 32) - left) < left * 0.01f);
        CHECK(std::abs(fixture.Read(31, 30 - distance) - left) < left * 0.01f);
        CHECK(std::abs(fixture.Read(32, 33 + distance) - left) < left * 0.01f);
    }
    for (const int x : {18, 4})
    {
        CAPTURE(x);
        const float normal = glowAt(1.0f, x);
        CHECK(glowAt(2.0f, x) > normal * 1.05f);
        CHECK(glowAt(0.0f, x) < normal * 0.97f);
        CHECK(std::abs(glowAt(-1.0f, x) - glowAt(0.0f, x)) < 1e-6f);
    }
}

TEST_CASE("A single very bright pixel glows like a much dimmer one, so that it does not flicker")
{
    const TestHelpers::ScopedLog log;
    Fixture fixture;
    fixture.Resize(64, 64);
    fixture.FillSquare({31, 31}, 1, 10000.0f);

    fixture.Apply({.Threshold = 0.0f, .SoftKnee = 0.0f, .Intensity = 1.0f});

    CHECK(fixture.Read(28, 31) < 1.0f);
    CHECK(fixture.Read(28, 31) > 0.0f);
}
