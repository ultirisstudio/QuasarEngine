#pragma once

/// \file
/// \brief The connection between a window and the graphics API that draws in it.
/// \author Kurogenshi
/// \date 2026-11-10

#include <quasar/renderer/GraphicsAPI.h>

namespace Quasar
{
    /// @brief Connection between a window and its graphics API, independent from the API.
    ///
    /// The window creates its context from WindowSpecification::Graphics and owns it. Each graphics API has its own
    /// class in `platform/<api>`, for example OpenGLContext. Code that needs the API itself converts the context to that
    /// class after checking GetAPI().
    class GraphicsContext
    {
    public:
        GraphicsContext() = default;
        virtual ~GraphicsContext() = default;

        GraphicsContext(const GraphicsContext&) = delete;
        GraphicsContext& operator=(const GraphicsContext&) = delete;
        GraphicsContext(GraphicsContext&&) = delete;
        GraphicsContext& operator=(GraphicsContext&&) = delete;

        /// @brief Returns the graphics API of the context.
        virtual GraphicsAPI GetAPI() const = 0;

        /// @brief Shows the image drawn since the last call in the window.
        virtual void SwapBuffers() = 0;

        /// @brief Enables or disables vertical synchronization, which makes SwapBuffers() wait for the monitor.
        virtual void SetVSync(bool enabled) = 0;
    };
}
