#include <quasar/events/EventDispatcher.h>
#include <quasar/events/WindowEvent.h>

#include <doctest/doctest.h>

#include <array>
#include <cstdint>

using Quasar::EventCategory;
using Quasar::EventType;
using Quasar::WindowCloseEvent;
using Quasar::WindowFocusEvent;
using Quasar::WindowMinimizeEvent;
using Quasar::WindowResizeEvent;

TEST_CASE("Window events have their own type")
{
    CHECK(WindowCloseEvent().GetType() == EventType::WindowClose);
    CHECK(WindowResizeEvent(1280, 720).GetType() == EventType::WindowResize);
    CHECK(WindowFocusEvent(true).GetType() == EventType::WindowFocus);
    CHECK(WindowMinimizeEvent(true).GetType() == EventType::WindowMinimize);
}

TEST_CASE("All window events share the window category")
{
    const WindowCloseEvent close;
    const WindowResizeEvent resize(1280, 720);
    const WindowFocusEvent focus(true);
    const WindowMinimizeEvent minimize(true);

    const auto events = std::to_array<const Quasar::Event*>({&close, &resize, &focus, &minimize});

    for (const Quasar::Event* event : events)
    {
        CHECK(event->IsInCategory(EventCategory::Window));
        CHECK_FALSE(event->IsInCategory(EventCategory::Input));
    }
}

TEST_CASE("A resize event carries the new size")
{
    const WindowResizeEvent event(1920, 1080);

    CHECK(event.GetWidth() == 1920);
    CHECK(event.GetHeight() == 1080);
    CHECK(event.ToString() == "WindowResize: 1920x1080");
}

TEST_CASE("A focus event tells whether the window gained or lost the focus")
{
    const WindowFocusEvent gained(true);
    const WindowFocusEvent lost(false);

    CHECK(gained.IsFocused());
    CHECK_FALSE(lost.IsFocused());
    CHECK(gained.ToString() == "WindowFocus: gained");
    CHECK(lost.ToString() == "WindowFocus: lost");
}

TEST_CASE("A minimize event tells whether the window was minimized or restored")
{
    const WindowMinimizeEvent minimized(true);
    const WindowMinimizeEvent restored(false);

    CHECK(minimized.IsMinimized());
    CHECK_FALSE(restored.IsMinimized());
    CHECK(minimized.ToString() == "WindowMinimize: minimized");
    CHECK(restored.ToString() == "WindowMinimize: restored");
}

TEST_CASE("A close event is printed with its name")
{
    CHECK(WindowCloseEvent().ToString() == "WindowClose");
}

TEST_CASE("A window event reaches the handler of its type")
{
    WindowResizeEvent event(800, 600);
    Quasar::EventDispatcher dispatcher(event);
    std::uint32_t receivedWidth = 0;

    dispatcher.Dispatch<WindowCloseEvent>([](WindowCloseEvent&) { return true; });
    dispatcher.Dispatch<WindowResizeEvent>(
        [&](WindowResizeEvent& resize)
        {
            receivedWidth = resize.GetWidth();
            return true;
        });

    CHECK(receivedWidth == 800);
    CHECK(event.IsHandled());
}
