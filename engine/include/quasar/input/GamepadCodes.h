#pragma once

/// \file
/// \brief Codes of the gamepads, of their buttons, of their axes and of their touchpads, independent from SDL.
/// \author Kurogenshi
/// \date 2026-11-06

#include <cstddef>
#include <cstdint>

namespace Quasar
{
    /// @brief Number of gamepads that can be connected at the same time.
    inline constexpr std::size_t MaxGamepads = 16;

    /// @brief Number of buttons of a gamepad, including the extra buttons that only some gamepads have.
    inline constexpr std::size_t GamepadButtonCount = 26;

    /// @brief Number of axes of a gamepad.
    inline constexpr std::size_t GamepadAxisCount = 6;

    /// @brief Number of touch sensors that a gamepad can have.
    inline constexpr std::size_t GamepadTouchSensorCount = 4;

    /// @brief Number of touchpads that a gamepad can have. A gamepad with more touchpads only gives the first ones.
    inline constexpr std::size_t MaxGamepadTouchpads = 2;

    /// @brief Number of fingers that a touchpad of a gamepad can follow at the same time. A touchpad that follows more
    /// fingers only gives the first ones.
    inline constexpr std::size_t MaxGamepadTouchpadFingers = 2;

    /// @brief Identifies a connected gamepad, from 0 to MaxGamepads - 1. Written as `GamepadId{0}` for the first one.
    ///
    /// A gamepad keeps its id while it stays connected. A new gamepad takes the lowest free id, from the next frame
    /// after the gamepad that had it was disconnected.
    enum class GamepadId : std::uint8_t
    {
    };

    /// @brief Kind of gamepad, for example to show the names or the icons of its buttons.
    ///
    /// The values are the same as the SDL gamepad types, so an SDL type converts with `static_cast<GamepadType>(type)`.
    enum class GamepadType : std::uint8_t
    {
        Unknown = 0,         ///< The kind of gamepad is not known.
        Standard = 1,        ///< A gamepad with the buttons of an Xbox controller, from another maker.
        Xbox360 = 2,         ///< Xbox 360 controller.
        XboxOne = 3,         ///< Xbox One or Xbox Series controller.
        PlayStation3 = 4,    ///< PlayStation 3 controller.
        PlayStation4 = 5,    ///< PlayStation 4 controller, DualShock 4.
        PlayStation5 = 6,    ///< PlayStation 5 controller, DualSense.
        SwitchPro = 7,       ///< Nintendo Switch Pro controller.
        JoyConLeft = 8,      ///< Left Joy-Con of the Nintendo Switch, held on its own.
        JoyConRight = 9,     ///< Right Joy-Con of the Nintendo Switch, held on its own.
        JoyConPair = 10,     ///< Both Joy-Cons of the Nintendo Switch, used as one gamepad.
        GameCube = 11,       ///< Nintendo GameCube controller.
        SteamController = 12 ///< Steam Controller.
    };

    /// @brief Button of a gamepad, at the position of the buttons of an Xbox controller.
    ///
    /// The same position is used on every gamepad: GamepadButton::A is the bottom face button, which is Cross on a
    /// PlayStation controller. The values are the same as the SDL gamepad buttons, so an SDL button converts with
    /// `static_cast<GamepadButton>(button)`.
    ///
    /// The buttons from Misc1 are only on some gamepads. On the other gamepads, they are never pressed. On the Steam
    /// controllers, SDL gives the touch sensors of the sticks and of the grips as Misc3 to Misc6. The engine reads them as
    /// GamepadTouchSensor instead, so these buttons are never pressed on these gamepads.
    enum class GamepadButton : std::uint8_t
    {
        A = 0,             ///< Bottom face button: A on Xbox, Cross on PlayStation.
        B = 1,             ///< Right face button: B on Xbox, Circle on PlayStation.
        X = 2,             ///< Left face button: X on Xbox, Square on PlayStation.
        Y = 3,             ///< Top face button: Y on Xbox, Triangle on PlayStation.
        Back = 4,          ///< Back or View button, Share on PlayStation.
        Guide = 5,         ///< Central button with the logo of the controller.
        Start = 6,         ///< Start or Menu button, Options on PlayStation.
        LeftThumb = 7,     ///< Press on the left stick.
        RightThumb = 8,    ///< Press on the right stick.
        LeftBumper = 9,    ///< Left shoulder button.
        RightBumper = 10,  ///< Right shoulder button.
        DPadUp = 11,       ///< Up on the directional pad.
        DPadDown = 12,     ///< Down on the directional pad.
        DPadLeft = 13,     ///< Left on the directional pad.
        DPadRight = 14,    ///< Right on the directional pad.
        Misc1 = 15,        ///< Extra button: Share on Xbox Series, microphone on PlayStation 5, Capture on Switch, quick access on Steam Controller.
        RightPaddle1 = 16, ///< Upper back button under the right hand: P1 on Xbox Elite, R4 on Steam Controller.
        LeftPaddle1 = 17,  ///< Upper back button under the left hand: P3 on Xbox Elite, L4 on Steam Controller.
        RightPaddle2 = 18, ///< Lower back button under the right hand: P2 on Xbox Elite, R5 on Steam Controller.
        LeftPaddle2 = 19,  ///< Lower back button under the left hand: P4 on Xbox Elite, L5 on Steam Controller.
        Touchpad = 20,     ///< Press on the touchpad on PlayStation, press on the left touchpad on the new Steam Controller.
        Misc2 = 21,        ///< Extra button: press on the right touchpad on the new Steam Controller.
        Misc3 = 22,        ///< Extra button.
        Misc4 = 23,        ///< Extra button.
        Misc5 = 24,        ///< Extra button.
        Misc6 = 25         ///< Extra button.
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

    /// @brief Sensor that feels a thumb or a hand without a press. Only the Steam controllers have them: the Steam
    /// Controller, the Steam Deck and the HORI Steam controller, which has no grip sensors.
    enum class GamepadTouchSensor : std::uint8_t
    {
        LeftStick = 0,  ///< A thumb rests on the left stick.
        RightStick = 1, ///< A thumb rests on the right stick.
        LeftGrip = 2,   ///< The left hand holds the back of the gamepad.
        RightGrip = 3   ///< The right hand holds the back of the gamepad.
    };
}
