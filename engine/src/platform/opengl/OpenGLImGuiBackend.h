#pragma once

#include <imgui/ImGuiBackend.h>

#include <quasar/renderer/GraphicsContext.h>

namespace Quasar
{
    class OpenGLImGuiBackend final : public ImGuiBackend
    {
    public:
        bool Init(Window& window) override;
        void Shutdown() override;
        void NewFrame() override;
        void Render() override;

    private:
        GraphicsContext* m_Context = nullptr;
    };
}
