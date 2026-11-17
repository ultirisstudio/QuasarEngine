#pragma once

/// \file
/// \brief The OpenGL 4.5 core context of a window.
/// \author Kurogenshi
/// \date 2026-11-10

#include <quasar/core/Base.h>
#include <quasar/renderer/GraphicsAPI.h>
#include <quasar/renderer/GraphicsContext.h>

struct SDL_Window;
struct SDL_GLContextState;

namespace Quasar
{
    /// @brief OpenGL 4.5 core context of a window, created with SDL. The OpenGL functions are loaded with glad.
    ///
    /// A window created with GraphicsAPI::OpenGL owns one, which Window::GetGraphicsContext() returns.
    class OpenGLContext final : public GraphicsContext
    {
    public:
        /// @brief Sets the attributes of the next OpenGL contexts that SDL creates: OpenGL 4.5 core, double buffered,
        /// with a depth and a stencil buffer. Call it before the window is created.
        static void SetAttributes();

        /// @brief Creates the context of a window, makes it current and loads the OpenGL functions.
        /// @param window The SDL window, created with SDL_WINDOW_OPENGL.
        /// @return The context, or nullptr if the driver does not provide OpenGL 4.5 core. The reason is logged as an
        /// engine error.
        static Scope<OpenGLContext> Create(SDL_Window* window);

        ~OpenGLContext() override;

        OpenGLContext(const OpenGLContext&) = delete;
        OpenGLContext& operator=(const OpenGLContext&) = delete;
        OpenGLContext(OpenGLContext&&) = delete;
        OpenGLContext& operator=(OpenGLContext&&) = delete;

        /// @brief Returns GraphicsAPI::OpenGL.
        GraphicsAPI GetAPI() const override { return GraphicsAPI::OpenGL; }

        /// @brief Makes the context current and clears the whole window.
        /// @return Always true: an OpenGL window always has an image to draw.
        bool BeginFrame() override;

        /// @brief Shows the image drawn since the last call in the window.
        void SwapBuffers() override;

        /// @brief Enables or disables vertical synchronization.
        void SetVSync(bool enabled) override;

        /// @brief Returns the SDL OpenGL context, for the code that needs to call SDL or OpenGL directly.
        SDL_GLContextState* GetNativeContext() const { return m_Context; }

    private:
        OpenGLContext(SDL_Window* window, SDL_GLContextState* context);

        SDL_Window* m_Window = nullptr;
        SDL_GLContextState* m_Context = nullptr;
    };
}
