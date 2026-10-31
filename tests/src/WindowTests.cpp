#include "NullWindow.h"

#include <quasar/core/KeyCodes.h>
#include <quasar/core/MouseCodes.h>
#include <quasar/events/Event.h>
#include <quasar/events/KeyEvent.h>
#include <quasar/events/MouseEvent.h>
#include <quasar/events/WindowEvent.h>

#include <doctest/doctest.h>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <string>
#include <vector>

namespace
{
    void RecordEvents(Quasar::Window& window, std::vector<std::string>& events)
    {
        window.SetEventCallback([&events](Quasar::Event& event) { events.push_back(event.ToString()); });
    }

    bool Contains(const std::vector<std::string>& events, const Quasar::Event& event)
    {
        return std::ranges::find(events, event.ToString()) != events.end();
    }
}

TEST_CASE("A window is created with the settings of its specification")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window =
        TestHelpers::CreateNullWindow({.Title = "Test", .Width = 640, .Height = 480, .VSync = false});

    REQUIRE(window != nullptr);
    CHECK(window->GetTitle() == "Test");
    CHECK(window->GetWidth() == 640);
    CHECK(window->GetHeight() == 480);
    CHECK_FALSE(window->IsVSync());
    CHECK_FALSE(window->ShouldClose());
    CHECK(window->GetNativeWindow() != nullptr);
}

TEST_CASE("Closing a window only asks it to close")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);

    window->Close();

    CHECK(window->ShouldClose());
}

TEST_CASE("Windows can be destroyed and created again")
{
    const TestHelpers::ScopedLog log;
    Quasar::Scope<Quasar::Window> first = TestHelpers::CreateNullWindow({.Title = "First"});
    Quasar::Scope<Quasar::Window> second = TestHelpers::CreateNullWindow({.Title = "Second"});
    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    first.reset();
    CHECK(second->GetWidth() == 1280);

    second.reset();
    const Quasar::Scope<Quasar::Window> third = TestHelpers::CreateNullWindow({.Title = "Third"});
    REQUIRE(third != nullptr);
    CHECK(third->GetTitle() == "Third");
}

TEST_CASE("Resizing a window sends a resize event with the new size")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);
    std::vector<std::string> events;
    RecordEvents(*window, events);

    glfwSetWindowSize(window->GetNativeWindow(), 800, 600);

    CHECK(Contains(events, Quasar::WindowResizeEvent(800, 600)));
    CHECK(window->GetWidth() == 800);
    CHECK(window->GetHeight() == 600);
}

TEST_CASE("Minimizing and restoring a window sends minimize events")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);
    std::vector<std::string> events;
    RecordEvents(*window, events);

    glfwIconifyWindow(window->GetNativeWindow());
    REQUIRE(Contains(events, Quasar::WindowMinimizeEvent(true)));
    CHECK_FALSE(Contains(events, Quasar::WindowMinimizeEvent(false)));

    glfwRestoreWindow(window->GetNativeWindow());
    CHECK(Contains(events, Quasar::WindowMinimizeEvent(false)));
}

TEST_CASE("The keyboard callbacks send key events")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);
    std::vector<std::string> events;
    RecordEvents(*window, events);

    TestHelpers::SendKey(*window, GLFW_KEY_A, GLFW_PRESS);
    TestHelpers::SendKey(*window, GLFW_KEY_A, GLFW_REPEAT);
    TestHelpers::SendKey(*window, GLFW_KEY_A, GLFW_RELEASE);
    TestHelpers::SendKey(*window, GLFW_KEY_UNKNOWN, GLFW_PRESS);
    TestHelpers::SendCharacter(*window, U'q');

    const std::vector<std::string> expected{
        Quasar::KeyPressedEvent(Quasar::Key::A).ToString(),
        Quasar::KeyPressedEvent(Quasar::Key::A, true).ToString(),
        Quasar::KeyReleasedEvent(Quasar::Key::A).ToString(),
        Quasar::KeyTypedEvent(U'q').ToString(),
    };
    CHECK(events == expected);
}

TEST_CASE("The mouse callbacks send mouse events")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);
    std::vector<std::string> events;
    RecordEvents(*window, events);

    TestHelpers::SendMouseButton(*window, GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
    TestHelpers::SendMouseButton(*window, GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE);
    TestHelpers::SendCursorPosition(*window, 12.5, 34.0);
    TestHelpers::SendScroll(*window, 0.0, -1.0);

    const std::vector<std::string> expected{
        Quasar::MouseButtonPressedEvent(Quasar::MouseButton::Left).ToString(),
        Quasar::MouseButtonReleasedEvent(Quasar::MouseButton::Left).ToString(),
        Quasar::MouseMovedEvent(12.5f, 34.0f).ToString(),
        Quasar::MouseScrolledEvent(0.0f, -1.0f).ToString(),
    };
    CHECK(events == expected);
}

TEST_CASE("The focus and close callbacks send window events")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);
    std::vector<std::string> events;
    RecordEvents(*window, events);

    TestHelpers::SendFocus(*window, false);
    TestHelpers::SendFocus(*window, true);
    TestHelpers::SendCloseRequest(*window);

    const std::vector<std::string> expected{
        Quasar::WindowFocusEvent(false).ToString(),
        Quasar::WindowFocusEvent(true).ToString(),
        Quasar::WindowCloseEvent().ToString(),
    };
    CHECK(events == expected);
}

TEST_CASE("A window without an event callback ignores its events")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);
    std::vector<std::string> events;
    RecordEvents(*window, events);

    window->SetEventCallback({});
    TestHelpers::SendKey(*window, GLFW_KEY_A, GLFW_PRESS);

    CHECK(events.empty());
}