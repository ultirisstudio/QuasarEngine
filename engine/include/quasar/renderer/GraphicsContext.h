#pragma once

/// \file
/// \brief The connection between a window and the graphics API that draws in it.
/// \author Kurogenshi
/// \date 2026-11-10

#include <quasar/renderer/GraphicsAPI.h>

#include <array>

namespace Quasar
{
    /// @brief Connection between a window and its graphics API, independent from the API.
    ///
    /// The window creates its context from WindowSpecification::Graphics and owns it. Each graphics API has its own
    /// class in `platform/<api>`, for example OpenGLContext. Code that needs the API itself converts the context to that
    /// class after checking GetAPI().
    ///
    /// Each frame, the application calls BeginFrame(), draws if it returned true, then the window calls SwapBuffers().
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

        /// @brief Prepares the next image of the window and clears it with ClearColor, before anything is drawn.
        /// @return False when the window has no image to draw this frame, for example while its size is 0. Nothing must
        /// be drawn then.
        virtual bool BeginFrame() = 0;

        /// @brief Shows the image drawn since the last BeginFrame() in the window. A context can skip it when
        /// BeginFrame() was not called or returned false.
        virtual void SwapBuffers() = 0;

        /// @brief Enables or disables vertical synchronization, which makes SwapBuffers() wait for the monitor.
        virtual void SetVSync(bool enabled) = 0;

        /// @brief Color of an image before anything is drawn, as red, green, blue and alpha from 0 to 1.
        static constexpr std::array<float, 4> ClearColor{0.08f, 0.08f, 0.09f, 1.0f};
    };
}
