#include <imgui/ImGuiBackend.h>

#ifdef QUASAR_OPENGL
#include <platform/opengl/OpenGLImGuiBackend.h>
#endif

#ifdef QUASAR_VULKAN
#include <platform/vulkan/VulkanImGuiBackend.h>
#endif

namespace Quasar
{
    Scope<ImGuiBackend> ImGuiBackend::Create(GraphicsAPI api)
    {
        switch (api)
        {
#ifdef QUASAR_OPENGL
            case GraphicsAPI::OpenGL:
                return CreateScope<OpenGLImGuiBackend>();
#else
            case GraphicsAPI::OpenGL:
#endif
#ifdef QUASAR_VULKAN
            case GraphicsAPI::Vulkan:
                return CreateScope<VulkanImGuiBackend>();
#else
            case GraphicsAPI::Vulkan:
#endif
            case GraphicsAPI::None:
                break;
        }
        return nullptr;
    }
}
