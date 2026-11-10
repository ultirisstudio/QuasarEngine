#include <imgui/ImGuiBackend.h>

#include <platform/opengl/OpenGLImGuiBackend.h>

namespace Quasar
{
    Scope<ImGuiBackend> ImGuiBackend::Create(GraphicsAPI api)
    {
        switch (api)
        {
            case GraphicsAPI::OpenGL:
                return CreateScope<OpenGLImGuiBackend>();
            case GraphicsAPI::None:
                break;
        }
        return nullptr;
    }
}
