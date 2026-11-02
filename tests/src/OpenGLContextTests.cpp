#include "NullWindow.h"
#include "ScopedEnvironmentVariable.h"

#include <quasar/core/Application.h>
#include <quasar/core/Layer.h>
#include <quasar/core/Log.h>
#include <quasar/core/MemoryLogSink.h>
#include <quasar/core/PlatformDetection.h>
#include <quasar/core/Window.h>

#include <doctest/doctest.h>

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// These tests need a real OpenGL driver without a display: the Mesa software driver through EGL, only on Linux
#ifdef QUASAR_PLATFORM_LINUX

namespace
{
    std::optional<std::string> FindMessage(std::string_view start)
    {
        for (const Quasar::LogMessage& message : Quasar::Log::GetMemorySink()->GetMessages())
        {
            if (message.Text.starts_with(start))
            {
                return message.Text;
            }
        }
        return std::nullopt;
    }

    bool HasMessageContaining(std::string_view text)
    {
        const std::vector<Quasar::LogMessage> messages = Quasar::Log::GetMemorySink()->GetMessages();
        return std::ranges::any_of(messages, [text](const Quasar::LogMessage& message) { return message.Text.find(text) != std::string::npos; });
    }

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

TEST_CASE("A window gets an OpenGL 4.5 core context")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindowWithContext();
    REQUIRE(window != nullptr);

    const std::string prefix = "OpenGL version: ";
    const std::optional<std::string> version = FindMessage(prefix);
    REQUIRE(version.has_value());

    const std::string_view number = std::string_view(*version).substr(prefix.size(), 3);
    CAPTURE(*version);
    CHECK((number == "4.5" || number == "4.6"));
}

TEST_CASE("The GPU vendor, renderer and OpenGL version are logged when the context is created")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindowWithContext();
    REQUIRE(window != nullptr);

    CHECK(FindMessage("OpenGL vendor: ").has_value());
    CHECK(FindMessage("OpenGL renderer: ").has_value());
    CHECK(FindMessage("OpenGL version: ").has_value());
    CHECK_FALSE(HasMessageContaining("unknown"));
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
    CHECK_FALSE(FindMessage("GLFW error").has_value());
}

TEST_CASE("No window is created when OpenGL 4.5 is not available")
{
    const TestHelpers::ScopedLog log;
    const TestHelpers::ScopedEnvironmentVariable version("MESA_GL_VERSION_OVERRIDE", "3.3");

    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindowWithContext({.Title = "Old driver"});

    CHECK(window == nullptr);
    CHECK(HasMessageContaining("OpenGL 4.5"));
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
    CHECK_FALSE(FindMessage("GLFW error").has_value());
}

#endif
