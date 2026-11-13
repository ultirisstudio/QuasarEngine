#include <renderer/GraphicsContextFactory.h>

#include <quasar/log/Log.h>

#ifdef QUASAR_OPENGL
#include <quasar/platform/opengl/OpenGLContext.h>
#endif

#ifdef QUASAR_VULKAN
#include <quasar/platform/vulkan/VulkanContext.h>
#endif

namespace Quasar
{
    SDL_WindowFlags PrepareGraphicsWindow(GraphicsAPI api)
    {
        switch (api)
        {
#ifdef QUASAR_OPENGL
            case GraphicsAPI::OpenGL:
                OpenGLContext::SetAttributes();
                return SDL_WINDOW_OPENGL;
#else
            case GraphicsAPI::OpenGL:
#endif
#ifdef QUASAR_VULKAN
            case GraphicsAPI::Vulkan:
                return SDL_WINDOW_VULKAN;
#else
            case GraphicsAPI::Vulkan:
#endif
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
#ifdef QUASAR_OPENGL
                return OpenGLContext::Create(window);
#else
                QUASAR_ENGINE_ERROR("Cannot create an OpenGL context: the engine was built without OpenGL");
                break;
#endif
            case GraphicsAPI::Vulkan:
#ifdef QUASAR_VULKAN
                return VulkanContext::Create(window);
#else
                QUASAR_ENGINE_ERROR("Cannot create a Vulkan context: the engine was built without the Vulkan SDK");
                break;
#endif
            case GraphicsAPI::None:
                break;
        }
        return nullptr;
    }
}
