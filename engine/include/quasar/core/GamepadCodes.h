#pragma once

/// \file
/// \brief Codes of the gamepads, of their buttons and of their axes, independent from GLFW.
/// \author Kurogenshi
/// \date 2026-11-06

#include <cstddef>
#include <cstdint>

namespace Quasar
{
    /// @brief Number of gamepads that can be connected at the same time.
    inline constexpr std::size_t MaxGamepads = 16;

    /// @brief Number of buttons of a gamepad.
    inline constexpr std::size_t GamepadButtonCount = 15;

    /// @brief Number of axes of a gamepad.
    inline constexpr std::size_t GamepadAxisCount = 6;

    /// @brief Identifies a connected gamepad, from 0 to MaxGamepads - 1. Written as `GamepadId{0}` for the first one.
    ///
    /// A gamepad keeps its id while it stays connected. The values are the same as the GLFW joystick ids.
    enum class GamepadId : std::uint8_t
    {
    };

    /// @brief Button of a gamepad, at the position of the buttons of an Xbox controller.
    ///
    /// The same position is used on every gamepad: GamepadButton::A is the bottom face button, which is Cross on a
    /// PlayStation controller. The values are the same as the GLFW gamepad button tokens.
    enum class GamepadButton : std::uint8_t
    {
        A = 0,           ///< Bottom face button: A on Xbox, Cross on PlayStation.
        B = 1,           ///< Right face button: B on Xbox, Circle on PlayStation.
        X = 2,           ///< Left face button: X on Xbox, Square on PlayStation.
        Y = 3,           ///< Top face button: Y on Xbox, Triangle on PlayStation.
        LeftBumper = 4,  ///< Left shoulder button.
        RightBumper = 5, ///< Right shoulder button.
        Back = 6,        ///< Back or View button, Share on PlayStation.
        Start = 7,       ///< Start or Menu button, Options on PlayStation.
        Guide = 8,       ///< Central button with the logo of the controller.
        LeftThumb = 9,   ///< Press on the left stick.
        RightThumb = 10, ///< Press on the right stick.
        DPadUp = 11,     ///< Up on the directional pad.
        DPadRight = 12,  ///< Right on the directional pad.
        DPadDown = 13,   ///< Down on the directional pad.
        DPadLeft = 14    ///< Left on the directional pad.
    };

    /// @brief Axis of a gamepad. The values are the same as the GLFW gamepad axis tokens.
    ///
    /// The stick axes go from -1 to 1: -1 is left on the X axes and up on the Y axes. The triggers go from 0, at rest,
    /// to 1, fully pressed.
    enum class GamepadAxis : std::uint8_t
    {
        LeftX = 0,       ///< Horizontal axis of the left stick.
        LeftY = 1,       ///< Vertical axis of the left stick.
        RightX = 2,      ///< Horizontal axis of the right stick.
        RightY = 3,      ///< Vertical axis of the right stick.
        LeftTrigger = 4, ///< Left trigger.
        RightTrigger = 5 ///< Right trigger.
    };
}
