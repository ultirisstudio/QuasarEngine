#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"
#include "helpers/ScopedEnvironmentVariable.h"
#include "helpers/TestLayers.h"

#include <quasar/core/Application.h>
#include <quasar/core/PlatformDetection.h>
#include <quasar/core/Window.h>
#include <quasar/log/MemoryLogSink.h>

#include <doctest/doctest.h>

#include <SDL3/SDL_video.h>
#include <glad/gl.h>

#include <utility>

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
    CHECK(SDL_GL_GetCurrentWindow() == window->GetNativeWindow());
    CHECK(SDL_GL_GetCurrentContext() == window->GetNativeContext());
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
