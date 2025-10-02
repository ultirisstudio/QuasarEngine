#pragma once

#include "qepch.h"
#include "QuasarEngine/Events/Event.h"
#include "QuasarEngine/Renderer/GraphicsContext.h"
#include <glm/glm.hpp>

struct GLFWwindow;

namespace QuasarEngine {

    class Window
    {
    public:
        using EventCallbackFn = std::function<void(Event&)>;

        struct WindowData
        {
            std::string   Title;
            unsigned int  Width, Height;
            bool          VSync;
            glm::uvec2    MousePos;
            EventCallbackFn EventCallback;

            WindowData(const std::string& title = "Quasar Engine",
                uint32_t width = 1280,
                uint32_t height = 720,
                bool vsync = false)
                : Title(title), Width(width), Height(height), VSync(vsync), MousePos(glm::uvec2(0u, 0u)) {
            }
        };

        Window();
        ~Window();

        void PollEvents();
        void WaitEvents(double timeoutSeconds);

        void BeginFrame();
        void EndFrame();

        void Resize(unsigned int width, unsigned int height);

        unsigned int GetWidth()  const noexcept { return m_Data.Width; }
        unsigned int GetHeight() const noexcept { return m_Data.Height; }

        template<typename T>
        T* GetContextAs() const { return dynamic_cast<T*>(m_Context.get()); }

        void SetVSync(bool enabled);
        bool IsVSync() const noexcept { return m_Data.VSync; }

        void SetInputMode(bool cursorDisabled, bool rawMouseMotion);
        void SetCursorVisibility(bool visible);

        void SetMousePosition(float x, float y);

        bool WaitEventsTimeout(double seconds);

        void SetMaximized(bool maximized);

        void* GetNativeWindow() const noexcept { return m_Window; }

        void SetTitle(const std::string& title);

		glm::uvec2 GetMousePosition() const { return m_Data.MousePos; }

        void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }

    private:
        void Shutdown();

    private:
        GLFWwindow* m_Window = nullptr;
        WindowData  m_Data{};
        std::unique_ptr<GraphicsContext> m_Context;

        static int s_GLFWRefCount;
    };
}
