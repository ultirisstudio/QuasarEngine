#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"
#include "helpers/ScopedEnvironmentVariable.h"
#include "helpers/TestLayers.h"

#include <quasar/core/Application.h>
#include <quasar/core/Layer.h>
#include <quasar/core/PlatformDetection.h>
#include <quasar/core/Window.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/platform/opengl/OpenGLContext.h>
#include <quasar/renderer/GpuProfiler.h>
#include <quasar/renderer/GraphicsAPI.h>
#include <quasar/renderer/GraphicsContext.h>
#include <quasar/renderer/Texture.h>
#include <quasar/renderer/TimestampQuery.h>
#include <quasar/time/Timestep.h>

#include <doctest/doctest.h>

#include <SDL3/SDL_video.h>
#include <glad/gl.h>
#include <glm/vec2.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

TEST_CASE("A window gets a current OpenGL 4.5 core context")
{
    const TestHelpers::ScopedLog log;

    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();

    REQUIRE(window != nullptr);
    GLint major = 0;
    GLint minor = 0;
    GLint profile = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
    CAPTURE(major);
    CAPTURE(minor);
    CHECK((major > 4 || (major == 4 && minor >= 5)));
    CHECK((profile & GL_CONTEXT_CORE_PROFILE_BIT) != 0);
    CHECK(window->GetGraphicsAPI() == Quasar::GraphicsAPI::OpenGL);
    const auto* context = dynamic_cast<const Quasar::OpenGLContext*>(window->GetGraphicsContext());
    REQUIRE(context != nullptr);
    CHECK(SDL_GL_GetCurrentWindow() == window->GetNativeWindow());
    CHECK(SDL_GL_GetCurrentContext() == context->GetNativeContext());
    CHECK(context->GetWindowSize() == glm::uvec2(window->GetWidth(), window->GetHeight()));
    CHECK(context->GetWindowSize().x > 0);
    GLint samples = 0;
    GLint colorSamples = 0;
    glGetIntegerv(GL_MAX_SAMPLES, &samples);
    glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &colorSamples);
    CHECK(context->GetMaxSamples() == static_cast<std::uint32_t>(std::min(samples, colorSamples)));
    GLfloat anisotropy = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &anisotropy);
    CHECK(context->GetMaxAnisotropy() == std::max(anisotropy, 1.0f));
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("A context makes its white texture once, with one white pixel")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    Quasar::GraphicsContext& context = *window->GetGraphicsContext();

    Quasar::Texture2D* white = context.GetWhiteTexture();

    REQUIRE(white != nullptr);
    CHECK(context.GetWhiteTexture() == white);
    CHECK(white->GetSpecification().Width == 1);
    CHECK(white->GetSpecification().Height == 1);
    white->Bind(3);
    glActiveTexture(GL_TEXTURE3);
    GLint texture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
    std::array<std::uint8_t, 4> pixel{};
    glGetTextureImage(static_cast<GLuint>(texture), 0, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLsizei>(pixel.size()), pixel.data());
    CHECK(pixel == std::array<std::uint8_t, 4>{255, 255, 255, 255});
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("A window with a context swaps its buffers and changes VSync without errors")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext({.VSync = false});
    REQUIRE(window != nullptr);

    window->Update();
    window->SetVSync(true);
    window->Update();

    CHECK(window->IsVSync());
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

#ifdef QUASAR_PLATFORM_LINUX
TEST_CASE("No window is created when OpenGL 4.5 is not available")
{
    const TestHelpers::ScopedLog log;
    const TestHelpers::ScopedEnvironmentVariable version("MESA_GL_VERSION_OVERRIDE", "3.3");

    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext({.Title = "Old driver"});

    CHECK(window == nullptr);
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}
#endif

TEST_CASE("An application runs its main loop with an OpenGL context")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateApplicationWithContext();
    REQUIRE(application != nullptr);
    auto layer = Quasar::CreateScope<TestHelpers::ClosingLayer>();
    const TestHelpers::ClosingLayer& closing = *layer;
    application->PushLayer(std::move(layer));

    application->Run();

    CHECK(closing.GetUpdates() == 3);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("A timestamp query gives the time of the graphics card once the graphics card reaches it")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    Quasar::GraphicsContext& context = *window->GetGraphicsContext();
    const Quasar::Scope<Quasar::TimestampQuery> first = context.CreateTimestampQuery();
    const Quasar::Scope<Quasar::TimestampQuery> second = context.CreateTimestampQuery();
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    CHECK_FALSE(first->GetNanoseconds().has_value());

    first->Record();
    context.ClearWindow({0.0f, 0.0f, 0.0f, 1.0f});
    second->Record();
    glFinish();

    const std::optional<std::uint64_t> begin = first->GetNanoseconds();
    const std::optional<std::uint64_t> end = second->GetNanoseconds();
    REQUIRE(begin.has_value());
    REQUIRE(end.has_value());
    CHECK(*end >= *begin);
    CHECK(glGetError() == GL_NO_ERROR);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

namespace
{
    // Waits for the graphics card at the start of each frame, so that the times of the previous frame are ready
    class MeasuredLayer final : public Quasar::Layer
    {
    public:
        MeasuredLayer() : Layer("Measured") {}

        void OnUpdate(Quasar::Timestep /*timestep*/) override { glFinish(); }

        void OnRender() override
        {
            const Quasar::GpuSection section(&Quasar::Application::Get().GetWindow().GetGraphicsContext()->GetProfiler(), "Layer");
            Quasar::Application::Get().GetWindow().GetGraphicsContext()->ClearWindow({0.0f, 0.0f, 0.0f, 1.0f});
        }
    };
}

TEST_CASE("The application measures each rendered frame on the graphics card, with its ImGui windows")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateApplicationWithContext();
    REQUIRE(application != nullptr);
    Quasar::GpuProfiler& profiler = application->GetWindow().GetGraphicsContext()->GetProfiler();
    profiler.SetEnabled(true);
    application->PushLayer(Quasar::CreateScope<MeasuredLayer>());
    application->PushLayer(Quasar::CreateScope<TestHelpers::ClosingLayer>(3));

    application->Run();

    std::vector<std::string> sections;
    for (const Quasar::GpuTiming& timing : profiler.GetTimings())
    {
        sections.push_back(timing.Name);
    }
    const bool hasImGui = application->GetImGuiLayer() != nullptr;
    CHECK(sections == (hasImGui ? std::vector<std::string>{"Layer", "ImGui"} : std::vector<std::string>{"Layer"}));
    CHECK(profiler.GetFrameMilliseconds() > 0.0);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}
