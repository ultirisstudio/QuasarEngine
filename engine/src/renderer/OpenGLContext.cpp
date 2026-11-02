#include "renderer/OpenGLContext.h"

#include <glad/gl.h>

#include <GLFW/glfw3.h>

#include <quasar/core/Log.h>

namespace Quasar
{
    namespace
    {
        constexpr int RequiredMajorVersion = 4;
        constexpr int RequiredMinorVersion = 5;

        const char* GetString(GLenum name)
        {
            const GLubyte* text = glGetString(name);
            // OpenGL returns its strings as unsigned characters, which hold the same bytes as characters
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            return text != nullptr ? reinterpret_cast<const char*>(text) : "unknown";
        }
    }

    OpenGLContext::OpenGLContext(GLFWwindow* window) : m_Window(window) {}

    void OpenGLContext::SetWindowHints()
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, RequiredMajorVersion);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, RequiredMinorVersion);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    }

    Scope<OpenGLContext> OpenGLContext::Create(GLFWwindow* window)
    {
        glfwMakeContextCurrent(window);

        const int version = gladLoadGL(glfwGetProcAddress);
        if (version == 0)
        {
            QUASAR_ENGINE_ERROR(
                "Cannot load the OpenGL functions. OpenGL {}.{} is required: update the graphics driver",
                RequiredMajorVersion, RequiredMinorVersion);
            return nullptr;
        }

        if (GLAD_GL_VERSION_4_5 == 0)
        {
            QUASAR_ENGINE_ERROR("OpenGL {}.{} is required, but the graphics driver only provides OpenGL {}.{}: update "
                                "the graphics driver",
                                RequiredMajorVersion, RequiredMinorVersion, GLAD_VERSION_MAJOR(version),
                                GLAD_VERSION_MINOR(version));
            return nullptr;
        }

        QUASAR_ENGINE_INFO("OpenGL vendor: {}", GetString(GL_VENDOR));
        QUASAR_ENGINE_INFO("OpenGL renderer: {}", GetString(GL_RENDERER));
        QUASAR_ENGINE_INFO("OpenGL version: {}", GetString(GL_VERSION));

        return Scope<OpenGLContext>(new OpenGLContext(window));
    }

    void OpenGLContext::SwapBuffers()
    {
        glfwSwapBuffers(m_Window);
    }

    void OpenGLContext::SetVSync(bool enabled)
    {
        glfwMakeContextCurrent(m_Window);
        glfwSwapInterval(enabled ? 1 : 0);
    }
}
