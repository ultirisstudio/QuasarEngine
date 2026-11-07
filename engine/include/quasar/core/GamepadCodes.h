#pragma once

/// \file
/// \brief Codes of the gamepads, of their buttons and of their axes, independent from SDL.
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
    /// A gamepad keeps its id while it stays connected. A new gamepad takes the lowest free id, from the next frame
    /// after the gamepad that had it was disconnected.
    enum class GamepadId : std::uint8_t
    {
    };

    /// @brief Button of a gamepad, at the position of the buttons of an Xbox controller.
    ///
    /// The same position is used on every gamepad: GamepadButton::A is the bottom face button, which is Cross on a
    /// PlayStation controller. The values are the same as the SDL gamepad buttons, so an SDL button converts with
    /// `static_cast<GamepadButton>(button)`.
    enum class GamepadButton : std::uint8_t
    {
        A = 0,            ///< Bottom face button: A on Xbox, Cross on PlayStation.
        B = 1,            ///< Right face button: B on Xbox, Circle on PlayStation.
        X = 2,            ///< Left face button: X on Xbox, Square on PlayStation.
        Y = 3,            ///< Top face button: Y on Xbox, Triangle on PlayStation.
        Back = 4,         ///< Back or View button, Share on PlayStation.
        Guide = 5,        ///< Central button with the logo of the controller.
        Start = 6,        ///< Start or Menu button, Options on PlayStation.
        LeftThumb = 7,    ///< Press on the left stick.
        RightThumb = 8,   ///< Press on the right stick.
        LeftBumper = 9,   ///< Left shoulder button.
        RightBumper = 10, ///< Right shoulder button.
        DPadUp = 11,      ///< Up on the directional pad.
        DPadDown = 12,    ///< Down on the directional pad.
        DPadLeft = 13,    ///< Left on the directional pad.
        DPadRight = 14    ///< Right on the directional pad.
    };

    /// @brief Axis of a gamepad. The values are the same as the SDL gamepad axes, so an SDL axis converts with
    /// `static_cast<GamepadAxis>(axis)`.
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
