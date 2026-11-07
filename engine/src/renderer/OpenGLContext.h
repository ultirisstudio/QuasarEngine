#pragma once

#include <quasar/core/Base.h>

struct SDL_Window;
struct SDL_GLContextState;

namespace Quasar
{
    class OpenGLContext
    {
    public:
        static void SetAttributes();

        static Scope<OpenGLContext> Create(SDL_Window* window);

        OpenGLContext(const OpenGLContext&) = delete;
        OpenGLContext& operator=(const OpenGLContext&) = delete;
        OpenGLContext(OpenGLContext&&) = delete;
        OpenGLContext& operator=(OpenGLContext&&) = delete;
        ~OpenGLContext();

        void SwapBuffers();

        void SetVSync(bool enabled);

        SDL_GLContextState* GetNativeContext() const { return m_Context; }

    private:
        OpenGLContext(SDL_Window* window, SDL_GLContextState* context);

        SDL_Window* m_Window = nullptr;
        SDL_GLContextState* m_Context = nullptr;
    };
}