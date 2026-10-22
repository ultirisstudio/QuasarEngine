#pragma once

/// \file
/// \brief Codes of the mouse buttons, independent from GLFW.
/// \author Kurogenshi
/// \date 2026-10-23

#include <cstdint>

namespace Quasar
{
    /// @brief Button of the mouse.
    ///
    /// The values are the same as the GLFW mouse button tokens, so a GLFW button converts with
    /// `static_cast<MouseButton>(button)`.
    enum class MouseButton : std::uint8_t
    {
        Left = 0,    ///< Left button.
        Right = 1,   ///< Right button.
        Middle = 2,  ///< Middle button.
        Button3 = 3, ///< First extra button, often the back button on the side of the mouse.
        Button4 = 4, ///< Second extra button, often the forward button on the side of the mouse.
        Button5 = 5, ///< Extra button.
        Button6 = 6, ///< Extra button.
        Button7 = 7  ///< Extra button.
    };
}
