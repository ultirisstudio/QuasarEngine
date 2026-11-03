#include "LogAssertions.h"
#include "NullWindow.h"
#include "ScopedEnvironmentVariable.h"

#include <quasar/core/Application.h>
#include <quasar/core/Layer.h>
#include <quasar/core/MemoryLogSink.h>
#include <quasar/core/PlatformDetection.h>
#include <quasar/core/Window.h>

#include <doctest/doctest.h>

#include <GLFW/glfw3.h>

#include <utility>

// These tests need a real OpenGL driver without a display: the Mesa software driver through EGL, only on Linux
#ifdef QUASAR_PLATFORM_LINUX

namespace
{
    class ClosingLayer : public Quasar::Layer
    {
    public:
        ClosingLayer() : Layer("Closing") {}

        void OnUpdate(Quasar::Timestep /*timestep*/) override
        {
            ++Updates;
            if (Updates == 3)
            {
                Quasar::Application::Get().Close();
            }
        }

        int Updates = 0;
    };
}

TEST_CASE("A window gets a current OpenGL 4.5 core context")
{
    const TestHelpers::ScopedLog log;

    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindowWithContext();

    REQUIRE(window != nullptr);
    GLFWwindow* handle = window->GetNativeWindow();
    const int major = glfwGetWindowAttrib(handle, GLFW_CONTEXT_VERSION_MAJOR);
    const int minor = glfwGetWindowAttrib(handle, GLFW_CONTEXT_VERSION_MINOR);
    CAPTURE(major);
    CAPTURE(minor);
    CHECK((major > 4 || (major == 4 && minor >= 5)));
    CHECK(glfwGetWindowAttrib(handle, GLFW_OPENGL_PROFILE) == GLFW_OPENGL_CORE_PROFILE);
    CHECK(glfwGetCurrentContext() == handle);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("A window with a context swaps its buffers and changes VSync without errors")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindowWithContext({.VSync = false});
    REQUIRE(window != nullptr);

    window->Update();
    window->SetVSync(true);
    window->Update();

    CHECK(window->IsVSync());
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("No window is created when OpenGL 4.5 is not available")
{
    const TestHelpers::ScopedLog log;
    const TestHelpers::ScopedEnvironmentVariable version("MESA_GL_VERSION_OVERRIDE", "3.3");

    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindowWithContext({.Title = "Old driver"});

    CHECK(window == nullptr);
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("An application runs its main loop with an OpenGL context")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplicationWithContext();
    REQUIRE(application != nullptr);
    auto layer = Quasar::CreateScope<ClosingLayer>();
    const ClosingLayer& closing = *layer;
    application->PushLayer(std::move(layer));

    application->Run();

    CHECK(closing.Updates == 3);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

#endif