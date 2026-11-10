#include <quasar/events/GamepadEvent.h>

#include <doctest/doctest.h>
#include <glm/vec2.hpp>

#include <array>
#include <set>
#include <string>
#include <vector>

using Quasar::EventCategory;
using Quasar::EventType;
using Quasar::GamepadAxis;
using Quasar::GamepadAxisMovedEvent;
using Quasar::GamepadButton;
using Quasar::GamepadButtonPressedEvent;
using Quasar::GamepadButtonReleasedEvent;
using Quasar::GamepadConnectedEvent;
using Quasar::GamepadDisconnectedEvent;
using Quasar::GamepadFingerDownEvent;
using Quasar::GamepadFingerMovedEvent;
using Quasar::GamepadFingerUpEvent;
using Quasar::GamepadId;

namespace
{
    constexpr glm::vec2 Center{0.5f, 0.5f};
}

TEST_CASE("Gamepad events have their own type")
{
    CHECK(GamepadConnectedEvent(GamepadId{0}, "Pad").GetType() == EventType::GamepadConnected);
    CHECK(GamepadDisconnectedEvent(GamepadId{0}).GetType() == EventType::GamepadDisconnected);
    CHECK(GamepadButtonPressedEvent(GamepadId{0}, GamepadButton::A).GetType() == EventType::GamepadButtonPressed);
    CHECK(GamepadButtonReleasedEvent(GamepadId{0}, GamepadButton::A).GetType() == EventType::GamepadButtonReleased);
    CHECK(GamepadAxisMovedEvent(GamepadId{0}, GamepadAxis::LeftX, 0.5f).GetType() == EventType::GamepadAxisMoved);
    CHECK(GamepadFingerDownEvent(GamepadId{0}, 0, 0, Center, 1.0f).GetType() == EventType::GamepadFingerDown);
    CHECK(GamepadFingerMovedEvent(GamepadId{0}, 0, 0, Center, 1.0f).GetType() == EventType::GamepadFingerMoved);
    CHECK(GamepadFingerUpEvent(GamepadId{0}, 0, 0, Center, 0.0f).GetType() == EventType::GamepadFingerUp);
}

TEST_CASE("All gamepad events are in the gamepad category and only buttons, axes and fingers are input")
{
    const GamepadConnectedEvent connected(GamepadId{0}, "Pad");
    const GamepadDisconnectedEvent disconnected(GamepadId{0});
    const GamepadButtonPressedEvent pressed(GamepadId{0}, GamepadButton::A);
    const GamepadButtonReleasedEvent released(GamepadId{0}, GamepadButton::A);
    const GamepadAxisMovedEvent moved(GamepadId{0}, GamepadAxis::LeftX, 0.5f);
    const GamepadFingerDownEvent fingerDown(GamepadId{0}, 0, 0, Center, 1.0f);
    const GamepadFingerMovedEvent fingerMoved(GamepadId{0}, 0, 0, Center, 1.0f);
    const GamepadFingerUpEvent fingerUp(GamepadId{0}, 0, 0, Center, 0.0f);

    for (const Quasar::Event* event :
         std::to_array<const Quasar::Event*>({&connected, &disconnected, &pressed, &released, &moved, &fingerDown, &fingerMoved, &fingerUp}))
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
    CHECK(fingerDown.IsInCategory(EventCategory::Input));
    CHECK(fingerMoved.IsInCategory(EventCategory::Input));
    CHECK(fingerUp.IsInCategory(EventCategory::Input));
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

TEST_CASE("Gamepad finger events carry the gamepad, the touchpad, the finger, the position and the pressure")
{
    const GamepadFingerMovedEvent moved(GamepadId{2}, 1, 0, glm::vec2{0.25f, 0.75f}, 0.5f);

    CHECK(moved.GetGamepad() == GamepadId{2});
    CHECK(moved.GetTouchpad() == 1);
    CHECK(moved.GetFinger() == 0);
    CHECK(moved.GetPosition() == glm::vec2{0.25f, 0.75f});
    CHECK(moved.GetPressure() == 0.5f);
}

TEST_CASE("Gamepad events are printed differently when their type or their data differ")
{
    const std::vector<std::string> texts{
        GamepadConnectedEvent(GamepadId{0}, "Pad").ToString(),
        GamepadConnectedEvent(GamepadId{1}, "Pad").ToString(),
        GamepadConnectedEvent(GamepadId{0}, "Other pad").ToString(),
        GamepadDisconnectedEvent(GamepadId{0}).ToString(),
        GamepadDisconnectedEvent(GamepadId{1}).ToString(),
        GamepadButtonPressedEvent(GamepadId{0}, GamepadButton::A).ToString(),
        GamepadButtonPressedEvent(GamepadId{1}, GamepadButton::A).ToString(),
        GamepadButtonPressedEvent(GamepadId{0}, GamepadButton::B).ToString(),
        GamepadButtonReleasedEvent(GamepadId{0}, GamepadButton::A).ToString(),
        GamepadButtonReleasedEvent(GamepadId{1}, GamepadButton::A).ToString(),
        GamepadButtonReleasedEvent(GamepadId{0}, GamepadButton::B).ToString(),
        GamepadAxisMovedEvent(GamepadId{0}, GamepadAxis::LeftX, 0.5f).ToString(),
        GamepadAxisMovedEvent(GamepadId{1}, GamepadAxis::LeftX, 0.5f).ToString(),
        GamepadAxisMovedEvent(GamepadId{0}, GamepadAxis::LeftY, 0.5f).ToString(),
        GamepadAxisMovedEvent(GamepadId{0}, GamepadAxis::LeftX, 0.25f).ToString(),
        GamepadFingerDownEvent(GamepadId{0}, 0, 0, Center, 1.0f).ToString(),
        GamepadFingerUpEvent(GamepadId{0}, 0, 0, Center, 1.0f).ToString(),
        GamepadFingerMovedEvent(GamepadId{0}, 0, 0, Center, 1.0f).ToString(),
        GamepadFingerMovedEvent(GamepadId{1}, 0, 0, Center, 1.0f).ToString(),
        GamepadFingerMovedEvent(GamepadId{0}, 1, 0, Center, 1.0f).ToString(),
        GamepadFingerMovedEvent(GamepadId{0}, 0, 1, Center, 1.0f).ToString(),
        GamepadFingerMovedEvent(GamepadId{0}, 0, 0, glm::vec2{0.25f, 0.5f}, 1.0f).ToString(),
        GamepadFingerMovedEvent(GamepadId{0}, 0, 0, glm::vec2{0.5f, 0.25f}, 1.0f).ToString(),
        GamepadFingerMovedEvent(GamepadId{0}, 0, 0, Center, 0.5f).ToString(),
    };

    CHECK(std::set<std::string>(texts.begin(), texts.end()).size() == texts.size());
}
