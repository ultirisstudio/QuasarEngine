#pragma once

#include <imgui/ImGuiBackend.h>

namespace Quasar
{
    class OpenGLImGuiBackend final : public ImGuiBackend
    {
    public:
        bool Init(Window& window) override;
        void Shutdown() override;
        void NewFrame() override;
        void Render() override;
    };
}
