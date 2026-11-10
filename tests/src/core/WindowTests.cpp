#include "helpers/NullWindow.h"

#include <quasar/events/Event.h>
#include <quasar/events/KeyEvent.h>
#include <quasar/events/MouseEvent.h>
#include <quasar/events/WindowEvent.h>
#include <quasar/input/KeyCodes.h>
#include <quasar/input/MouseCodes.h>
#include <quasar/renderer/GraphicsAPI.h>

#include <SDL3/SDL_scancode.h>
#include <doctest/doctest.h>

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

TEST_CASE("A window without a graphics API has no graphics context")
{
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);

    CHECK(window->GetGraphicsAPI() == Quasar::GraphicsAPI::None);
    CHECK(window->GetGraphicsContext() == nullptr);
}

TEST_CASE("A window is created with the settings of its specification")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow({.Title = "Test", .Width = 640, .Height = 480, .VSync = false});

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
    CHECK(second->GetWidth() == Quasar::WindowSpecification{}.Width);

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

    TestHelpers::SendResize(*window, 800, 600);
    window->Update();

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

    TestHelpers::SendMinimize(*window);
    window->Update();
    REQUIRE(Contains(events, Quasar::WindowMinimizeEvent(true)));
    CHECK_FALSE(Contains(events, Quasar::WindowMinimizeEvent(false)));

    TestHelpers::SendRestore(*window);
    window->Update();
    CHECK(Contains(events, Quasar::WindowMinimizeEvent(false)));
}

TEST_CASE("The keyboard events of SDL send key events")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);
    std::vector<std::string> events;
    RecordEvents(*window, events);

    TestHelpers::SendKey(*window, Quasar::Key::A, true);
    TestHelpers::SendKey(*window, Quasar::Key::A, true, true);
    TestHelpers::SendKey(*window, Quasar::Key::A, false);
    TestHelpers::SendKey(*window, static_cast<Quasar::Key>(SDL_SCANCODE_UNKNOWN), true);
    TestHelpers::SendText(*window, "q\xC3\xA9");
    window->Update();

    const std::vector<std::string> expected{
        Quasar::KeyPressedEvent(Quasar::Key::A).ToString(),  Quasar::KeyPressedEvent(Quasar::Key::A, true).ToString(),
        Quasar::KeyReleasedEvent(Quasar::Key::A).ToString(), Quasar::KeyTypedEvent(U'q').ToString(),
        Quasar::KeyTypedEvent(U'\u00E9').ToString(),
    };
    CHECK(events == expected);
}

TEST_CASE("The mouse events of SDL send mouse events")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);
    std::vector<std::string> events;
    RecordEvents(*window, events);

    TestHelpers::SendMouseButton(*window, Quasar::MouseButton::Left, true);
    TestHelpers::SendMouseButton(*window, Quasar::MouseButton::Left, false);
    TestHelpers::SendMouseMotion(*window, 12.5f, 34.0f);
    TestHelpers::SendScroll(*window, 0.0f, -1.0f);
    window->Update();

    const std::vector<std::string> expected{
        Quasar::MouseButtonPressedEvent(Quasar::MouseButton::Left).ToString(),
        Quasar::MouseButtonReleasedEvent(Quasar::MouseButton::Left).ToString(),
        Quasar::MouseMovedEvent(12.5f, 34.0f).ToString(),
        Quasar::MouseScrolledEvent(0.0f, -1.0f).ToString(),
    };
    CHECK(events == expected);
}

TEST_CASE("The focus and close events of SDL send window events")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);
    std::vector<std::string> events;
    RecordEvents(*window, events);

    TestHelpers::SendFocus(*window, false);
    TestHelpers::SendFocus(*window, true);
    TestHelpers::SendCloseRequest(*window);
    window->Update();

    const std::vector<std::string> expected{
        Quasar::WindowFocusEvent(false).ToString(),
        Quasar::WindowFocusEvent(true).ToString(),
        Quasar::WindowCloseEvent().ToString(),
    };
    CHECK(events == expected);
    CHECK(window->ShouldClose());
}

TEST_CASE("A window without an event callback ignores its events")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);
    std::vector<std::string> events;
    RecordEvents(*window, events);

    window->SetEventCallback({});
    TestHelpers::SendKey(*window, Quasar::Key::A, true);
    window->Update();

    CHECK(events.empty());
}
