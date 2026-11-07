#include <quasar/events/EventDispatcher.h>
#include <quasar/events/GamepadEvent.h>

#include <doctest/doctest.h>

#include <array>

using Quasar::EventCategory;
using Quasar::EventType;
using Quasar::GamepadAxis;
using Quasar::GamepadAxisMovedEvent;
using Quasar::GamepadButton;
using Quasar::GamepadButtonPressedEvent;
using Quasar::GamepadButtonReleasedEvent;
using Quasar::GamepadConnectedEvent;
using Quasar::GamepadDisconnectedEvent;
using Quasar::GamepadId;

TEST_CASE("Gamepad events have their own type")
{
    CHECK(GamepadConnectedEvent(GamepadId{0}, "Pad").GetType() == EventType::GamepadConnected);
    CHECK(GamepadDisconnectedEvent(GamepadId{0}).GetType() == EventType::GamepadDisconnected);
    CHECK(GamepadButtonPressedEvent(GamepadId{0}, GamepadButton::A).GetType() == EventType::GamepadButtonPressed);
    CHECK(GamepadButtonReleasedEvent(GamepadId{0}, GamepadButton::A).GetType() == EventType::GamepadButtonReleased);
    CHECK(GamepadAxisMovedEvent(GamepadId{0}, GamepadAxis::LeftX, 0.5f).GetType() == EventType::GamepadAxisMoved);
}

TEST_CASE("All gamepad events are in the gamepad category and only buttons and axes are input")
{
    const GamepadConnectedEvent connected(GamepadId{0}, "Pad");
    const GamepadDisconnectedEvent disconnected(GamepadId{0});
    const GamepadButtonPressedEvent pressed(GamepadId{0}, GamepadButton::A);
    const GamepadButtonReleasedEvent released(GamepadId{0}, GamepadButton::A);
    const GamepadAxisMovedEvent moved(GamepadId{0}, GamepadAxis::LeftX, 0.5f);

    for (const Quasar::Event* event : std::to_array<const Quasar::Event*>({&connected, &disconnected, &pressed, &released, &moved}))
    {
        CHECK(event->IsInCategory(EventCategory::Gamepad));
        CHECK_FALSE(event->IsInCategory(EventCategory::Keyboard));
        CHECK_FALSE(event->IsInCategory(EventCategory::Mouse));
        CHECK_FALSE(event->IsInCategory(EventCategory::Window));
    }
    CHECK_FALSE(connected.IsInCategory(EventCategory::Input));
    CHECK_FALSE(disconnected.IsInCategory(EventCategory::Input));
    CHECK(pressed.IsInCategory(EventCategory::Input));
    CHECK(released.IsInCategory(EventCategory::Input));
    CHECK(moved.IsInCategory(EventCategory::Input));
}

TEST_CASE("Gamepad events carry the gamepad and their data")
{
    const GamepadConnectedEvent connected(GamepadId{3}, "Xbox Controller");
    const GamepadButtonPressedEvent pressed(GamepadId{1}, GamepadButton::Start);
    const GamepadButtonReleasedEvent released(GamepadId{2}, GamepadButton::DPadLeft);
    const GamepadAxisMovedEvent moved(GamepadId{4}, GamepadAxis::RightTrigger, 0.75f);

    CHECK(connected.GetGamepad() == GamepadId{3});
    CHECK(connected.GetGamepadName() == "Xbox Controller");
    CHECK(GamepadDisconnectedEvent(GamepadId{5}).GetGamepad() == GamepadId{5});
    CHECK(pressed.GetGamepad() == GamepadId{1});
    CHECK(pressed.GetButton() == GamepadButton::Start);
    CHECK(released.GetGamepad() == GamepadId{2});
    CHECK(released.GetButton() == GamepadButton::DPadLeft);
    CHECK(moved.GetGamepad() == GamepadId{4});
    CHECK(moved.GetAxis() == GamepadAxis::RightTrigger);
    CHECK(moved.GetValue() == 0.75f);
}

TEST_CASE("Gamepad events are printed with their data")
{
    CHECK(GamepadConnectedEvent(GamepadId{0}, "Xbox Controller").ToString() == "GamepadConnected: 0 (Xbox Controller)");
    CHECK(GamepadDisconnectedEvent(GamepadId{1}).ToString() == "GamepadDisconnected: 1");
    CHECK(GamepadButtonPressedEvent(GamepadId{0}, GamepadButton::B).ToString() == "GamepadButtonPressed: 0, 1");
    CHECK(GamepadButtonReleasedEvent(GamepadId{0}, GamepadButton::B).ToString() == "GamepadButtonReleased: 0, 1");
    CHECK(GamepadAxisMovedEvent(GamepadId{0}, GamepadAxis::LeftTrigger, 0.5f).ToString() == "GamepadAxisMoved: 0, 4, 0.5");
}

TEST_CASE("A gamepad event reaches the handler of its type")
{
    GamepadButtonPressedEvent event(GamepadId{0}, GamepadButton::Y);
    Quasar::EventDispatcher dispatcher(event);
    GamepadButton received = GamepadButton::A;

    dispatcher.Dispatch<GamepadButtonPressedEvent>(
        [&received](const GamepadButtonPressedEvent& pressed)
        {
            received = pressed.GetButton();
            return true;
        });

    CHECK(received == GamepadButton::Y);
    CHECK(event.IsHandled());
}
