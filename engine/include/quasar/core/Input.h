#pragma once

/// \file
/// \brief Functions that read the current state of the keyboard and the mouse.
/// \author Kurogenshi
/// \date 2026-10-25

#include <quasar/core/KeyCodes.h>
#include <quasar/core/MouseCodes.h>

#include <glm/vec2.hpp>

/// @brief Functions that read the current state of the keyboard and the mouse, as an alternative to events.
///
/// Use them for continuous actions, like moving a camera while a key is held down:
/// `if (Input::IsKeyPressed(Key::W)) { MoveForward(timestep); }`. Use events for single actions, like a click or a
/// shortcut, so that no press is missed between two frames.
///
/// The functions read the main window of the application, so an Application must exist. The state is the one known
/// at the last call to Window::Update(), which processes the messages of the operating system once per frame.
namespace Quasar::Input
{
    /// @brief Returns true if the key is held down.
    ///
    /// Key codes are physical keys: Key::W is the key at the position of W on QWERTY, which is Z on AZERTY.
    bool IsKeyPressed(Key key);

    /// @brief Returns true if the mouse button is held down.
    bool IsMouseButtonPressed(MouseButton button);

    /// @brief Returns the position of the cursor, in screen coordinates, from the top-left corner of the drawing
    /// area.
    ///
    /// Like MouseMovedEvent, the position is not in pixels on a scaled display.
    glm::vec2 GetMousePosition();
}
