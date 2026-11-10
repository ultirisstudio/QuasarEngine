#include <renderer/GraphicsContextFactory.h>

#include <quasar/platform/opengl/OpenGLContext.h>

namespace Quasar
{
    SDL_WindowFlags PrepareGraphicsWindow(GraphicsAPI api)
    {
        switch (api)
        {
            case GraphicsAPI::OpenGL:
                OpenGLContext::SetAttributes();
                return SDL_WINDOW_OPENGL;
            case GraphicsAPI::None:
                break;
        }
        return 0;
    }

    Scope<GraphicsContext> CreateGraphicsContext(GraphicsAPI api, SDL_Window* window)
    {
        switch (api)
        {
            case GraphicsAPI::OpenGL:
                return OpenGLContext::Create(window);
            case GraphicsAPI::None:
                break;
        }
        return nullptr;
    }
}
