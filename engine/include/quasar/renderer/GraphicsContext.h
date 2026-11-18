#pragma once

/// \file
/// \brief The connection between a window and the graphics API that draws in it.
/// \author Kurogenshi
/// \date 2026-11-10

#include <quasar/core/Base.h>
#include <quasar/renderer/Framebuffer.h>
#include <quasar/renderer/GraphicsAPI.h>
#include <quasar/renderer/Shader.h>

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

        /// @brief Creates a framebuffer that draws with the graphics API of the context.
        /// @return The framebuffer, or nullptr if the specification is not valid or if the graphics API cannot create it.
        /// The reason is logged as an engine error.
        [[nodiscard]] virtual Scope<Framebuffer> CreateFramebuffer(const FramebufferSpecification& specification) = 0;

        /// @brief Compiles a shader for the graphics API of the context.
        /// @return The shader, or nullptr if the specification is not valid or if a stage cannot be compiled or linked.
        /// The reason, with the messages of the compiler, is logged as an engine error.
        [[nodiscard]] virtual Scope<Shader> CreateShader(const ShaderSpecification& specification) = 0;
    };
}
