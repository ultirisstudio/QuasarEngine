#pragma once

#include <quasar/core/Base.h>
#include <quasar/renderer/GraphicsAPI.h>

namespace Quasar
{
    class Window;

    class ImGuiBackend
    {
    public:
        ImGuiBackend() = default;
        virtual ~ImGuiBackend() = default;

        ImGuiBackend(const ImGuiBackend&) = delete;
        ImGuiBackend& operator=(const ImGuiBackend&) = delete;
        ImGuiBackend(ImGuiBackend&&) = delete;
        ImGuiBackend& operator=(ImGuiBackend&&) = delete;

        static Scope<ImGuiBackend> Create(GraphicsAPI api);

        virtual bool Init(Window& window) = 0;
        virtual void Shutdown() = 0;
        virtual void NewFrame() = 0;
        virtual void Render() = 0;
    };
}
