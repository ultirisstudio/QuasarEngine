#include <imgui/ImGuiBackend.h>

#include <platform/opengl/OpenGLImGuiBackend.h>

#ifdef QUASAR_VULKAN
#include <platform/vulkan/VulkanImGuiBackend.h>
#endif

namespace Quasar
{
    Scope<ImGuiBackend> ImGuiBackend::Create(GraphicsAPI api)
    {
        switch (api)
        {
            case GraphicsAPI::OpenGL:
                return CreateScope<OpenGLImGuiBackend>();
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
