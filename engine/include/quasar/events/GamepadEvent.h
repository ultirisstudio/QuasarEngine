#pragma once

/// \file
/// \brief Events sent by the gamepads: connection, disconnection, buttons, axes and touchpads.
/// \author Kurogenshi
/// \date 2026-11-06

#include <quasar/core/GamepadCodes.h>
#include <quasar/events/Event.h>

#include <glm/vec2.hpp>
#include <spdlog/fmt/fmt.h>

#include <cstddef>
#include <string>
#include <utility>

namespace Quasar
{
    /// @brief Sent when a gamepad is connected, or when it is already connected as the application starts.
    class GamepadConnectedEvent final : public EventOf<EventType::GamepadConnected, EventCategory::Gamepad>
    {
    public:
        /// @brief Creates the event for the connected gamepad.
        /// @param gamepad The id that the gamepad keeps while it stays connected.
        /// @param name The name of the gamepad, for example "Xbox Controller".
        GamepadConnectedEvent(GamepadId gamepad, std::string name) : m_Gamepad(gamepad), m_Name(std::move(name)) {}

        /// @brief Returns the id of the gamepad.
        GamepadId GetGamepad() const { return m_Gamepad; }

        /// @brief Returns the name of the gamepad.
        const std::string& GetGamepadName() const { return m_Name; }

        /// @brief Returns the event with the id and the name, for example "GamepadConnected: 0 (Xbox Controller)".
        std::string ToString() const override { return fmt::format("{}: {} ({})", GetName(), static_cast<unsigned>(m_Gamepad), m_Name); }

    private:
        GamepadId m_Gamepad{};
        std::string m_Name;
    };

    /// @brief Sent when a gamepad is disconnected, after a released event for each of its buttons still held down.
    class GamepadDisconnectedEvent final : public EventOf<EventType::GamepadDisconnected, EventCategory::Gamepad>
    {
    public:
        /// @brief Creates the event for the disconnected gamepad.
        explicit GamepadDisconnectedEvent(GamepadId gamepad) : m_Gamepad(gamepad) {}

        /// @brief Returns the id the gamepad had. A gamepad connected later can get the same id.
        GamepadId GetGamepad() const { return m_Gamepad; }

        /// @brief Returns the event with the id, for example "GamepadDisconnected: 0".
        std::string ToString() const override { return fmt::format("{}: {}", GetName(), static_cast<unsigned>(m_Gamepad)); }

    private:
        GamepadId m_Gamepad{};
    };

    /// @brief Sent when a button of a gamepad is pressed.
    class GamepadButtonPressedEvent final : public EventOf<EventType::GamepadButtonPressed, EventCategory::Input | EventCategory::Gamepad>
    {
    public:
        /// @brief Creates the event for the pressed button.
        GamepadButtonPressedEvent(GamepadId gamepad, GamepadButton button) : m_Gamepad(gamepad), m_Button(button) {}

        /// @brief Returns the id of the gamepad.
        GamepadId GetGamepad() const { return m_Gamepad; }

        /// @brief Returns the pressed button.
        GamepadButton GetButton() const { return m_Button; }

        /// @brief Returns the event with the id and the button code, for example "GamepadButtonPressed: 0, 1".
        std::string ToString() const override
        {
            return fmt::format("{}: {}, {}", GetName(), static_cast<unsigned>(m_Gamepad), static_cast<unsigned>(m_Button));
        }

    private:
        GamepadId m_Gamepad{};
        GamepadButton m_Button{};
    };

    /// @brief Sent when a button of a gamepad is released.
    class GamepadButtonReleasedEvent final : public EventOf<EventType::GamepadButtonReleased, EventCategory::Input | EventCategory::Gamepad>
    {
    public:
        /// @brief Creates the event for the released button.
        GamepadButtonReleasedEvent(GamepadId gamepad, GamepadButton button) : m_Gamepad(gamepad), m_Button(button) {}

        /// @brief Returns the id of the gamepad.
        GamepadId GetGamepad() const { return m_Gamepad; }

        /// @brief Returns the released button.
        GamepadButton GetButton() const { return m_Button; }

        /// @brief Returns the event with the id and the button code, for example "GamepadButtonReleased: 0, 1".
        std::string ToString() const override
        {
            return fmt::format("{}: {}, {}", GetName(), static_cast<unsigned>(m_Gamepad), static_cast<unsigned>(m_Button));
        }

    private:
        GamepadId m_Gamepad{};
        GamepadButton m_Button{};
    };

    /// @brief Sent when an axis of a gamepad moves enough. See GamepadSettings for the dead zones and the threshold.
    class GamepadAxisMovedEvent final : public EventOf<EventType::GamepadAxisMoved, EventCategory::Input | EventCategory::Gamepad>
    {
    public:
        /// @brief Creates the event with the new value of the axis.
        /// @param gamepad The id of the gamepad.
        /// @param axis The axis that moved.
        /// @param value The new value: from -1 to 1 for a stick axis, from 0 to 1 for a trigger.
        GamepadAxisMovedEvent(GamepadId gamepad, GamepadAxis axis, float value) : m_Gamepad(gamepad), m_Axis(axis), m_Value(value) {}

        /// @brief Returns the id of the gamepad.
        GamepadId GetGamepad() const { return m_Gamepad; }

        /// @brief Returns the axis that moved.
        GamepadAxis GetAxis() const { return m_Axis; }

        /// @brief Returns the new value of the axis: from -1 to 1 for a stick axis, from 0 to 1 for a trigger.
        float GetValue() const { return m_Value; }

        /// @brief Returns the event with the id, the axis code and the value, for example "GamepadAxisMoved: 0, 4, 0.5".
        std::string ToString() const override
        {
            return fmt::format("{}: {}, {}, {}", GetName(), static_cast<unsigned>(m_Gamepad), static_cast<unsigned>(m_Axis), m_Value);
        }

    private:
        GamepadId m_Gamepad{};
        GamepadAxis m_Axis{};
        float m_Value = 0.0f;
    };

    /// @brief Base class of the events of a finger on a touchpad of a gamepad, which gives them their data.
    ///
    /// The finger events are GamepadFingerDownEvent, GamepadFingerMovedEvent and GamepadFingerUpEvent.
    template <EventType Type>
    class GamepadFingerEventOf : public EventOf<Type, EventCategory::Input | EventCategory::Gamepad>
    {
    public:
        /// @brief Creates the event for a finger on a touchpad.
        /// @param gamepad The id of the gamepad.
        /// @param touchpad The index of the touchpad on the gamepad.
        /// @param finger The index of the finger on the touchpad.
        /// @param position The position of the finger, from 0 to 1 on each axis, from the top-left corner of the touchpad.
        /// @param pressure The pressure of the finger, from 0 to 1.
        GamepadFingerEventOf(GamepadId gamepad, std::size_t touchpad, std::size_t finger, glm::vec2 position, float pressure)
            : m_Gamepad(gamepad), m_Touchpad(touchpad), m_Finger(finger), m_Position(position), m_Pressure(pressure)
        {
        }

        /// @brief Returns the id of the gamepad.
        GamepadId GetGamepad() const { return m_Gamepad; }

        /// @brief Returns the index of the touchpad on the gamepad.
        std::size_t GetTouchpad() const { return m_Touchpad; }

        /// @brief Returns the index of the finger on the touchpad.
        std::size_t GetFinger() const { return m_Finger; }

        /// @brief Returns the position of the finger, from 0 to 1 on each axis, from the top-left corner of the touchpad.
        glm::vec2 GetPosition() const { return m_Position; }

        /// @brief Returns the pressure of the finger, from 0 to 1.
        float GetPressure() const { return m_Pressure; }

        /// @brief Returns the event with the id, the touchpad, the finger, the position and the pressure, for example
        /// "GamepadFingerMoved: 0, 1, 0, 0.5, 0.25, 1".
        std::string ToString() const override
        {
            return fmt::format("{}: {}, {}, {}, {}, {}, {}", this->GetName(), static_cast<unsigned>(m_Gamepad), m_Touchpad, m_Finger, m_Position.x,
                               m_Position.y, m_Pressure);
        }

    private:
        GamepadId m_Gamepad{};
        std::size_t m_Touchpad = 0;
        std::size_t m_Finger = 0;
        glm::vec2 m_Position{0.0f, 0.0f};
        float m_Pressure = 0.0f;
    };

    /// @brief Sent when a finger touches a touchpad of a gamepad.
    class GamepadFingerDownEvent final : public GamepadFingerEventOf<EventType::GamepadFingerDown>
    {
    public:
        using GamepadFingerEventOf::GamepadFingerEventOf;
    };

    /// @brief Sent when a finger moves on a touchpad of a gamepad, or presses it harder or softer.
    class GamepadFingerMovedEvent final : public GamepadFingerEventOf<EventType::GamepadFingerMoved>
    {
    public:
        using GamepadFingerEventOf::GamepadFingerEventOf;
    };

    /// @brief Sent when a finger leaves a touchpad of a gamepad, at the last position where it was down, with a
    /// pressure of 0.
    class GamepadFingerUpEvent final : public GamepadFingerEventOf<EventType::GamepadFingerUp>
    {
    public:
        using GamepadFingerEventOf::GamepadFingerEventOf;
    };
}
