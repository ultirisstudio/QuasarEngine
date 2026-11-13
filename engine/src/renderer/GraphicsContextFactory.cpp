#include <renderer/GraphicsContextFactory.h>

#include <quasar/log/Log.h>
#include <quasar/platform/opengl/OpenGLContext.h>

#ifdef QUASAR_VULKAN
#include <quasar/platform/vulkan/VulkanContext.h>
#endif

namespace Quasar
{
    SDL_WindowFlags PrepareGraphicsWindow(GraphicsAPI api)
    {
        switch (api)
        {
            case GraphicsAPI::OpenGL:
                OpenGLContext::SetAttributes();
                return SDL_WINDOW_OPENGL;
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
                return OpenGLContext::Create(window);
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
