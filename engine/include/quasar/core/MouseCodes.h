#pragma once

/// \file
/// \brief Codes of the mouse buttons, independent from SDL.
/// \author Kurogenshi
/// \date 2026-10-23

#include <cstdint>

namespace Quasar
{
    /// @brief Button of the mouse.
    ///
    /// The values are the same as the SDL mouse buttons, so an SDL button converts with
    /// `static_cast<MouseButton>(button)`. Mice with more buttons can send other values.
    enum class MouseButton : std::uint8_t
    {
        Left = 1,   ///< Left button.
        Middle = 2, ///< Middle button, often the press on the wheel.
        Right = 3,  ///< Right button.
        Back = 4,   ///< First extra button, often the back button on the side of the mouse.
        Forward = 5 ///< Second extra button, often the forward button on the side of the mouse.
    };
}
