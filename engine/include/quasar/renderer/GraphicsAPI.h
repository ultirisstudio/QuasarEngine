#pragma once

/// \file
/// \brief The graphics APIs that the engine can draw with.
/// \author Kurogenshi
/// \date 2026-11-10

#include <cstdint>

namespace Quasar
{
    /// @brief Graphics API that a window draws with.
    ///
    /// The code of each API lives in its own folder, `platform/<api>`. The rest of the engine only uses the interfaces
    /// of the `renderer` folder, such as GraphicsContext, so that another API can be added without changing it.
    enum class GraphicsAPI : std::uint8_t
    {
        None,   ///< No graphics API: the window cannot draw. Only meant for the tests that run without a graphics driver.
        OpenGL, ///< OpenGL 4.5 core, on Windows and Linux.
        Vulkan  ///< Vulkan 1.3, on Windows and Linux. Only available when the engine is built with the Vulkan SDK.
    };
}
