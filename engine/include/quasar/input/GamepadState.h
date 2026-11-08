#pragma once

/// \file
/// \brief State of a gamepad: its buttons, its axes, the fingers on its touchpads and its touch sensors.
/// \author Kurogenshi
/// \date 2026-11-08

#include <quasar/input/GamepadCodes.h>

#include <glm/vec2.hpp>

#include <array>
#include <cstddef>

namespace Quasar
{
    /// @brief A finger on a touchpad of a gamepad.
    struct GamepadFinger
    {
        bool Down = false;              ///< True while the finger touches the touchpad.
        glm::vec2 Position{0.0f, 0.0f}; ///< Position from 0 to 1 on each axis, from the top-left corner of the touchpad.
        float Pressure = 0.0f;          ///< Pressure from 0 to 1, or 0 when the finger is not down.

        /// \cond
        bool operator==(const GamepadFinger&) const = default;
        /// \endcond
    };

    /// @brief The fingers of a touchpad of a gamepad, indexed from 0. A touchpad that follows fewer fingers than
    /// MaxGamepadTouchpadFingers never has the others down.
    struct GamepadTouchpad
    {
        std::array<GamepadFinger, MaxGamepadTouchpadFingers> Fingers{}; ///< State of each finger.
    };

    /// @brief State of the buttons, axes, touchpads and touch sensors of a gamepad.
    struct GamepadState
    {
        std::array<bool, GamepadButtonCount> Buttons{};               ///< True for each button held down, indexed by GamepadButton.
        std::array<float, GamepadAxisCount> Axes{};                   ///< Value of each axis, indexed by GamepadAxis.
        std::array<GamepadTouchpad, MaxGamepadTouchpads> Touchpads{}; ///< State of each touchpad, indexed from 0.
        std::array<bool, GamepadTouchSensorCount> TouchSensors{};     ///< True for each sensor touched, indexed by GamepadTouchSensor.

        /// @brief Returns true if the button is held down.
        bool IsPressed(GamepadButton button) const { return Buttons.at(static_cast<std::size_t>(button)); }

        /// @brief Returns the value of the axis.
        float GetAxis(GamepadAxis axis) const { return Axes.at(static_cast<std::size_t>(axis)); }

        /// @brief Returns true if the touch sensor is touched.
        bool IsTouched(GamepadTouchSensor sensor) const { return TouchSensors.at(static_cast<std::size_t>(sensor)); }
    };
}
