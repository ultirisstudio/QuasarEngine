#pragma once

/// \file
/// \brief Functions that read the current state of the keyboard, the mouse and the gamepads.
/// \author Kurogenshi
/// \date 2026-10-25

#include <quasar/input/GamepadCodes.h>
#include <quasar/input/Gamepads.h>
#include <quasar/input/KeyCodes.h>
#include <quasar/input/MouseCodes.h>

#include <glm/vec2.hpp>

#include <cstddef>

/// @brief Functions that read the current state of the keyboard, the mouse and the gamepads, as an alternative to
/// events.
///
/// Use them for continuous actions, like moving a camera while a key is held down:
/// `if (Input::IsKeyPressed(Key::W)) { MoveForward(timestep); }`. Use events for single actions, like a click or a
/// shortcut, so that no press is missed between two frames.
///
/// The functions read the main window and the gamepads of the application, so an Application must exist. The state is
/// the one known at the end of the last frame, when the messages of the operating system and the gamepads are read.
namespace Quasar::Input
{
    /// @brief Returns true if the key is held down.
    ///
    /// Key codes are physical keys: Key::W is the key at the position of W on QWERTY, which is Z on AZERTY.
    bool IsKeyPressed(Key key);

    /// @brief Returns true if the mouse button is held down.
    bool IsMouseButtonPressed(MouseButton button);

    /// @brief Returns the position of the cursor, from the top-left corner of the drawing area.
    ///
    /// The position is in the same unit as MouseMovedEvent: pixels on Windows and X11, and points on Wayland and
    /// macOS, where a scaled display has more pixels than points.
    glm::vec2 GetMousePosition();

    /// @brief Returns true if a gamepad is connected with this id. See Gamepads.
    bool IsGamepadConnected(GamepadId gamepad);

    /// @brief Returns true if the button of the gamepad is held down. False when the gamepad is not connected.
    bool IsGamepadButtonPressed(GamepadId gamepad, GamepadButton button);

    /// @brief Returns the value of an axis of the gamepad, after the dead zones. 0 when the gamepad is not connected.
    ///
    /// Stick axes go from -1 to 1, -1 being left or up. Triggers go from 0, at rest, to 1, fully pressed. See
    /// GamepadSettings for the dead zones.
    float GetGamepadAxis(GamepadId gamepad, GamepadAxis axis);

    /// @brief Returns a finger on a touchpad of the gamepad. See Gamepads::GetTouchpadFinger().
    ///
    /// The finger is not down when the gamepad is not connected, or when the touchpad or the finger does not exist.
    GamepadFinger GetGamepadTouchpadFinger(GamepadId gamepad, std::size_t touchpad, std::size_t finger);
}
