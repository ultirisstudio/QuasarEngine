#include "helpers/NullWindow.h"

#include <quasar/events/Event.h>
#include <quasar/events/KeyEvent.h>
#include <quasar/events/MouseEvent.h>
#include <quasar/events/WindowEvent.h>
#include <quasar/input/KeyCodes.h>
#include <quasar/input/MouseCodes.h>
#include <quasar/renderer/GraphicsAPI.h>

#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>
#include <doctest/doctest.h>

#include <algorithm>
#include <array>
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
    const TestHelpers::ScopedLog log;
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

TEST_CASE("Minimizing and restoring a window sends one minimize event each time its state changes")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);
    std::vector<std::string> events;
    RecordEvents(*window, events);

    TestHelpers::SendRestore(*window);
    TestHelpers::SendMinimize(*window);
    TestHelpers::SendMinimize(*window);
    TestHelpers::SendRestore(*window);
    TestHelpers::SendRestore(*window);
    window->Update();

    const std::vector<std::string> expected{
        Quasar::WindowMinimizeEvent(true).ToString(),
        Quasar::WindowMinimizeEvent(false).ToString(),
    };
    CHECK(events == expected);
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
    TestHelpers::SendKey(*window, static_cast<Quasar::Key>(Quasar::KeyCount), true);
    TestHelpers::SendText(*window, "q\xC3\xA9");
    window->Update();

    const std::vector<std::string> expected{
        Quasar::KeyPressedEvent(Quasar::Key::A).ToString(),  Quasar::KeyPressedEvent(Quasar::Key::A, true).ToString(),
        Quasar::KeyReleasedEvent(Quasar::Key::A).ToString(), Quasar::KeyTypedEvent(U'q').ToString(),
        Quasar::KeyTypedEvent(U'\u00E9').ToString(),
    };
    CHECK(events == expected);
}

TEST_CASE("A window starts the text input again when something else stops it")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);

    SDL_StopTextInput(window->GetNativeWindow());
    REQUIRE_FALSE(SDL_TextInputActive(window->GetNativeWindow()));
    window->Update();

    CHECK(SDL_TextInputActive(window->GetNativeWindow()));
}

TEST_CASE("A key or a mouse button outside their codes is never down")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);

    CHECK_FALSE(window->IsKeyDown(static_cast<Quasar::Key>(Quasar::KeyCount)));
    CHECK_FALSE(window->IsMouseButtonDown(static_cast<Quasar::MouseButton>(0)));
    CHECK_FALSE(window->IsMouseButtonDown(static_cast<Quasar::MouseButton>(33)));
}

TEST_CASE("A key that prints a character is named with this character")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);

    const std::string a = window->GetKeyName(Quasar::Key::A);
    const std::string b = window->GetKeyName(Quasar::Key::B);

    CHECK_FALSE(a.empty());
    CHECK_FALSE(b.empty());
    CHECK(a != b);
}

TEST_CASE("A key that prints no character has an empty name")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateNullWindow();
    REQUIRE(window != nullptr);

    for (const Quasar::Key key : std::to_array({Quasar::Key::Escape, Quasar::Key::Space, Quasar::Key::Delete, Quasar::Key::F1}))
    {
        CAPTURE(static_cast<int>(key));
        CHECK(window->GetKeyName(key).empty());
    }
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
    TestHelpers::SendScroll(*window, 2.0f, -1.0f, true);
    window->Update();

    const std::vector<std::string> expected{
        Quasar::MouseButtonPressedEvent(Quasar::MouseButton::Left).ToString(),
        Quasar::MouseButtonReleasedEvent(Quasar::MouseButton::Left).ToString(),
        Quasar::MouseMovedEvent(12.5f, 34.0f).ToString(),
        Quasar::MouseScrolledEvent(0.0f, -1.0f).ToString(),
        Quasar::MouseScrolledEvent(-2.0f, 1.0f).ToString(),
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
