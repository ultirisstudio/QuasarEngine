#include <quasar/events/EventDispatcher.h>
#include <quasar/events/MouseEvent.h>

#include <doctest/doctest.h>

using Quasar::EventCategory;
using Quasar::EventType;
using Quasar::MouseButton;
using Quasar::MouseButtonPressedEvent;
using Quasar::MouseButtonReleasedEvent;
using Quasar::MouseMovedEvent;
using Quasar::MouseScrolledEvent;

TEST_CASE("Mouse events have their own type")
{
    CHECK(MouseMovedEvent(0.0f, 0.0f).GetType() == EventType::MouseMoved);
    CHECK(MouseScrolledEvent(0.0f, 1.0f).GetType() == EventType::MouseScrolled);
    CHECK(MouseButtonPressedEvent(MouseButton::Left).GetType() == EventType::MouseButtonPressed);
    CHECK(MouseButtonReleasedEvent(MouseButton::Left).GetType() == EventType::MouseButtonReleased);
}

TEST_CASE("All mouse events share the mouse and input categories")
{
    const MouseMovedEvent moved(0.0f, 0.0f);
    const MouseScrolledEvent scrolled(0.0f, 1.0f);
    const MouseButtonPressedEvent pressed(MouseButton::Left);
    const MouseButtonReleasedEvent released(MouseButton::Left);

    const Quasar::Event* events[] = {&moved, &scrolled, &pressed, &released};

    for (const Quasar::Event* event : events)
    {
        CHECK(event->IsInCategory(EventCategory::Mouse));
        CHECK(event->IsInCategory(EventCategory::Input));
        CHECK_FALSE(event->IsInCategory(EventCategory::Keyboard));
        CHECK_FALSE(event->IsInCategory(EventCategory::Window));
    }
}

TEST_CASE("A move event carries the cursor position")
{
    const MouseMovedEvent event(640.0f, 360.5f);

    CHECK(event.GetX() == 640.0f);
    CHECK(event.GetY() == 360.5f);
    CHECK(event.ToString() == "MouseMoved: 640, 360.5");
}

TEST_CASE("A scroll event carries the scroll offsets")
{
    const MouseScrolledEvent event(-0.5f, 1.0f);

    CHECK(event.GetXOffset() == -0.5f);
    CHECK(event.GetYOffset() == 1.0f);
    CHECK(event.ToString() == "MouseScrolled: -0.5, 1");
}

TEST_CASE("Button events carry the mouse button code")
{
    const MouseButtonPressedEvent pressed(MouseButton::Right);
    const MouseButtonReleasedEvent released(MouseButton::Middle);

    CHECK(pressed.GetButton() == MouseButton::Right);
    CHECK(released.GetButton() == MouseButton::Middle);
    CHECK(pressed.ToString() == "MouseButtonPressed: 1");
    CHECK(released.ToString() == "MouseButtonReleased: 2");
}

TEST_CASE("A mouse event reaches the handler of its type")
{
    MouseButtonPressedEvent event(MouseButton::Left);
    Quasar::EventDispatcher dispatcher(event);
    MouseButton receivedButton = MouseButton::Right;

    dispatcher.Dispatch<MouseButtonReleasedEvent>([](MouseButtonReleasedEvent&) { return true; });
    dispatcher.Dispatch<MouseButtonPressedEvent>(
        [&](MouseButtonPressedEvent& pressed)
        {
            receivedButton = pressed.GetButton();
            return true;
        });

    CHECK(receivedButton == MouseButton::Left);
    CHECK(event.IsHandled());
}