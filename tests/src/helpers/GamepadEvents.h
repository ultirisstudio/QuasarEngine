#pragma once

#include <quasar/events/Event.h>
#include <quasar/events/EventDispatcher.h>
#include <quasar/events/GamepadEvent.h>
#include <quasar/input/GamepadCodes.h>
#include <quasar/input/Gamepads.h>

#include <glm/vec2.hpp>

#include <cstddef>
#include <vector>

namespace TestHelpers
{
    struct RecordedEvent
    {
        Quasar::EventType Type{};
        Quasar::GamepadId Gamepad{};
        Quasar::GamepadButton Button{};
        Quasar::GamepadAxis Axis{};
        float Value = 0.0f;
        std::size_t Touchpad = 0;
        std::size_t Finger = 0;
        glm::vec2 Position{0.0f, 0.0f};
        float Pressure = 0.0f;

        bool operator==(const RecordedEvent&) const = default;
    };

    inline RecordedEvent Connected(Quasar::GamepadId gamepad)
    {
        return {.Type = Quasar::EventType::GamepadConnected, .Gamepad = gamepad};
    }

    inline RecordedEvent Disconnected(Quasar::GamepadId gamepad)
    {
        return {.Type = Quasar::EventType::GamepadDisconnected, .Gamepad = gamepad};
    }

    inline RecordedEvent Pressed(Quasar::GamepadId gamepad, Quasar::GamepadButton button)
    {
        return {.Type = Quasar::EventType::GamepadButtonPressed, .Gamepad = gamepad, .Button = button};
    }

    inline RecordedEvent Released(Quasar::GamepadId gamepad, Quasar::GamepadButton button)
    {
        return {.Type = Quasar::EventType::GamepadButtonReleased, .Gamepad = gamepad, .Button = button};
    }

    inline RecordedEvent FingerDown(Quasar::GamepadId gamepad, std::size_t touchpad, std::size_t finger, glm::vec2 position, float pressure)
    {
        return {.Type = Quasar::EventType::GamepadFingerDown,
                .Gamepad = gamepad,
                .Touchpad = touchpad,
                .Finger = finger,
                .Position = position,
                .Pressure = pressure};
    }

    inline RecordedEvent FingerMoved(Quasar::GamepadId gamepad, std::size_t touchpad, std::size_t finger, glm::vec2 position, float pressure)
    {
        return {.Type = Quasar::EventType::GamepadFingerMoved,
                .Gamepad = gamepad,
                .Touchpad = touchpad,
                .Finger = finger,
                .Position = position,
                .Pressure = pressure};
    }

    inline RecordedEvent FingerUp(Quasar::GamepadId gamepad, std::size_t touchpad, std::size_t finger, glm::vec2 position)
    {
        return {.Type = Quasar::EventType::GamepadFingerUp, .Gamepad = gamepad, .Touchpad = touchpad, .Finger = finger, .Position = position};
    }

    template <typename FingerEvent>
    void RecordFinger(Quasar::EventDispatcher& dispatcher, std::vector<RecordedEvent>& events)
    {
        dispatcher.Dispatch<FingerEvent>(
            [&events](const FingerEvent& finger)
            {
                events.push_back({.Type = FingerEvent::GetStaticType(),
                                  .Gamepad = finger.GetGamepad(),
                                  .Touchpad = finger.GetTouchpad(),
                                  .Finger = finger.GetFinger(),
                                  .Position = finger.GetPosition(),
                                  .Pressure = finger.GetPressure()});
                return false;
            });
    }

    inline void RecordGamepadEvent(Quasar::Event& event, std::vector<RecordedEvent>& events)
    {
        Quasar::EventDispatcher dispatcher(event);
        dispatcher.Dispatch<Quasar::GamepadConnectedEvent>(
            [&events](const Quasar::GamepadConnectedEvent& connected)
            {
                events.push_back(Connected(connected.GetGamepad()));
                return false;
            });
        dispatcher.Dispatch<Quasar::GamepadDisconnectedEvent>(
            [&events](const Quasar::GamepadDisconnectedEvent& disconnected)
            {
                events.push_back(Disconnected(disconnected.GetGamepad()));
                return false;
            });
        dispatcher.Dispatch<Quasar::GamepadButtonPressedEvent>(
            [&events](const Quasar::GamepadButtonPressedEvent& pressed)
            {
                events.push_back(Pressed(pressed.GetGamepad(), pressed.GetButton()));
                return false;
            });
        dispatcher.Dispatch<Quasar::GamepadButtonReleasedEvent>(
            [&events](const Quasar::GamepadButtonReleasedEvent& released)
            {
                events.push_back(Released(released.GetGamepad(), released.GetButton()));
                return false;
            });
        dispatcher.Dispatch<Quasar::GamepadAxisMovedEvent>(
            [&events](const Quasar::GamepadAxisMovedEvent& moved)
            {
                events.push_back(
                    {.Type = Quasar::EventType::GamepadAxisMoved, .Gamepad = moved.GetGamepad(), .Axis = moved.GetAxis(), .Value = moved.GetValue()});
                return false;
            });
        RecordFinger<Quasar::GamepadFingerDownEvent>(dispatcher, events);
        RecordFinger<Quasar::GamepadFingerMovedEvent>(dispatcher, events);
        RecordFinger<Quasar::GamepadFingerUpEvent>(dispatcher, events);
    }

    inline std::vector<RecordedEvent> UpdateGamepads(Quasar::Gamepads& gamepads)
    {
        std::vector<RecordedEvent> events;
        gamepads.Update([&events](Quasar::Event& event) { RecordGamepadEvent(event, events); });
        return events;
    }
}
