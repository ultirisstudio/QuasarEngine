#include <quasar/platform/opengl/OpenGLContext.h>

#include <glad/gl.h>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>

#include <quasar/log/Log.h>

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

    OpenGLContext::OpenGLContext(SDL_Window* window, SDL_GLContextState* context) : m_Window(window), m_Context(context) {}

    OpenGLContext::~OpenGLContext()
    {
        SDL_GL_DestroyContext(m_Context);
    }

    void OpenGLContext::SetAttributes()
    {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, RequiredMajorVersion);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, RequiredMinorVersion);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    }

    Scope<OpenGLContext> OpenGLContext::Create(SDL_Window* window)
    {
        SDL_GLContext context = SDL_GL_CreateContext(window);
        if (context == nullptr)
        {
            QUASAR_ENGINE_ERROR("Cannot create an OpenGL {}.{} core context: {}. Update the graphics driver", RequiredMajorVersion, RequiredMinorVersion,
                                SDL_GetError());
            return nullptr;
        }

        Scope<OpenGLContext> created(new OpenGLContext(window, context));

        const int version = gladLoadGL(SDL_GL_GetProcAddress);
        if (version == 0)
        {
            QUASAR_ENGINE_ERROR("Cannot load the OpenGL functions. OpenGL {}.{} is required: update the graphics driver", RequiredMajorVersion,
                                RequiredMinorVersion);
            return nullptr;
        }

        if (GLAD_GL_VERSION_4_5 == 0)
        {
            QUASAR_ENGINE_ERROR("OpenGL {}.{} is required, but the graphics driver only provides OpenGL {}.{}: update the graphics driver",
                                RequiredMajorVersion, RequiredMinorVersion, GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));
            return nullptr;
        }

        QUASAR_ENGINE_INFO("OpenGL vendor: {}", GetString(GL_VENDOR));
        QUASAR_ENGINE_INFO("OpenGL renderer: {}", GetString(GL_RENDERER));
        QUASAR_ENGINE_INFO("OpenGL version: {}", GetString(GL_VERSION));

        return created;
    }

    bool OpenGLContext::BeginFrame()
    {
        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(m_Window, &width, &height);

        SDL_GL_MakeCurrent(m_Window, m_Context);
        glViewport(0, 0, width, height);
        glClearColor(ClearColor[0], ClearColor[1], ClearColor[2], ClearColor[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        return true;
    }

    void OpenGLContext::SwapBuffers()
    {
        SDL_GL_SwapWindow(m_Window);
    }

    void OpenGLContext::SetVSync(bool enabled)
    {
        SDL_GL_MakeCurrent(m_Window, m_Context);
        SDL_GL_SetSwapInterval(enabled ? 1 : 0);
    }
}
