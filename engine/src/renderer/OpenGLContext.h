#pragma once

#include <quasar/core/Base.h>

struct GLFWwindow;

namespace Quasar
{
    class OpenGLContext
    {
    public:
        static void SetWindowHints();

        static Scope<OpenGLContext> Create(GLFWwindow* window);

        OpenGLContext(const OpenGLContext&) = delete;
        OpenGLContext& operator=(const OpenGLContext&) = delete;
        OpenGLContext(OpenGLContext&&) = delete;
        OpenGLContext& operator=(OpenGLContext&&) = delete;
        ~OpenGLContext() = default;

        void SwapBuffers();

        void SetVSync(bool enabled);

    private:
        explicit OpenGLContext(GLFWwindow* window);

        GLFWwindow* m_Window = nullptr;
    };
}
