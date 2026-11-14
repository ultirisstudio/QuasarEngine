#pragma once

/// \file
/// \brief Application window, created with SDL.
/// \author Kurogenshi
/// \date 2026-10-23

#include <quasar/core/Base.h>
#include <quasar/events/Event.h>
#include <quasar/input/KeyCodes.h>
#include <quasar/input/MouseCodes.h>
#include <quasar/renderer/GraphicsAPI.h>
#include <quasar/renderer/GraphicsContext.h>

#include <glm/vec2.hpp>

#include <bitset>
#include <cstdint>
#include <functional>
#include <string>

struct SDL_Window;
union SDL_Event;

namespace Quasar
{
    /// @brief Settings used to create a window.
    ///
    /// Only the fields that differ from the defaults need a value: `Window::Create({.Title = "Editor"})`.
    struct WindowSpecification
    {
        std::string Title = "Quasar Engine"; ///< Title of the window.
        /// Width of the window when it is created: in pixels on Windows and X11, in points on Wayland and macOS, where a
        /// scaled display has more pixels than points. GetWidth() gives the size in pixels.
        std::uint32_t Width = 1280;
        std::uint32_t Height = 720; ///< Height of the window when it is created, in the same unit as Width.
        bool VSync = true;          ///< Whether vertical synchronization is enabled when it is created.

        /// Graphics API of the window, whose context is created with the window: OpenGL by default, and Vulkan on Android,
        /// which has no OpenGL. A window with GraphicsAPI::None cannot draw anything: it is only meant for tests that run
        /// without a graphics driver.
        GraphicsAPI Graphics = PlatformDetection::IsAndroid() ? GraphicsAPI::Vulkan : GraphicsAPI::OpenGL;

        /// Whether the window is shown when it is created. A hidden window still receives the events sent to it, which
        /// the tests use.
        bool Visible = true;
    };

    /// @brief Application window.
    ///
    /// A window is created with Create() and destroyed with its Scope. The video support of SDL is started with the
    /// first window and stopped with the last one.
    ///
    /// The window keeps the state of the keyboard and of the mouse from the events it receives, so the state and the
    /// events always agree. Input reads this state for the main window. The touches of the screen and of a pen send
    /// touch events and do not move the mouse.
    class Window
    {
    public:
        /// @brief Function that receives the events of the window.
        using EventCallback = std::function<void(Event&)>;

        /// @brief Function that receives every SDL event, before the window reads it.
        using NativeEventCallback = std::function<void(const SDL_Event&)>;

        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        /// @brief Creates a window and shows it, unless the specification asks for a hidden window.
        /// @param specification Title, size and settings of the window.
        /// @return The new window, or `nullptr` if it cannot be created. The reason is logged as an engine error.
        [[nodiscard]] static Scope<Window> Create(const WindowSpecification& specification);

        /// @brief Shows the rendered image and processes the messages of the operating system.
        ///
        /// Call it once per frame, after rendering. Every SDL event waiting is read: it goes to the native event
        /// callback first, then to the window it belongs to, which sends the matching engine event. The events that
        /// belong to no window, such as AppBackgroundEvent, go to the window that reads them.
        void Update();

        /// @brief Sets the function that receives every event of the window.
        void SetEventCallback(EventCallback callback);

        /// @brief Sets the function that receives every SDL event read by Update(), for the code that needs SDL
        /// directly. The ImGui layer uses it to give the events to ImGui.
        void SetNativeEventCallback(NativeEventCallback callback);

        /// @brief Returns true when the window was asked to close, by the user or by Close().
        bool ShouldClose() const;

        /// @brief Asks the window to close. ShouldClose() then returns true.
        void Close();

        /// @brief Enables or disables vertical synchronization.
        ///
        /// With VSync, the frame rate follows the refresh rate of the monitor, which prevents tearing.
        void SetVSync(bool enabled);

        /// @brief Returns true if vertical synchronization is enabled.
        bool IsVSync() const;

        /// @brief Returns the width of the drawing area, in pixels.
        std::uint32_t GetWidth() const;

        /// @brief Returns the height of the drawing area, in pixels.
        std::uint32_t GetHeight() const;

        /// @brief Returns the title of the window.
        const std::string& GetTitle() const;

        /// @brief Returns true if the key is held down, from the key events received by the window.
        bool IsKeyDown(Key key) const;

        /// @brief Returns true if the mouse button is held down, from the button events received by the window.
        bool IsMouseButtonDown(MouseButton button) const;

        /// @brief Returns the last position of the cursor received by the window, from the top-left corner of the
        /// drawing area.
        glm::vec2 GetMousePosition() const;

        /// @brief Returns the SDL window, for the code that needs to call SDL directly.
        SDL_Window* GetNativeWindow() const;

        /// @brief Returns the graphics API of the window, or GraphicsAPI::None when it has no graphics context.
        GraphicsAPI GetGraphicsAPI() const;

        /// @brief Returns the graphics context of the window, or nullptr when its graphics API is GraphicsAPI::None.
        GraphicsContext* GetGraphicsContext() const;

        /// @brief Returns the name of a key in the current keyboard layout, for example "z" for Key::W on AZERTY.
        /// @return The name of the key, or an empty string for keys that do not print a character, like Escape.
        std::string GetKeyName(Key key) const;

    private:
        Window(SDL_Window* handle, const WindowSpecification& specification);

        void ProcessEvent(const SDL_Event& event);
        void ProcessTouchEvent(const SDL_Event& event);
        void SendEvent(Event& event);

        SDL_Window* m_Handle = nullptr;
        Scope<GraphicsContext> m_Context;
        std::string m_Title;
        bool m_VSync = false;
        bool m_ShouldClose = false;
        bool m_Minimized = false;
        std::uint32_t m_Width = 0;
        std::uint32_t m_Height = 0;
        std::bitset<KeyCount> m_KeysDown;
        std::uint32_t m_MouseButtonsDown = 0;
        glm::vec2 m_MousePosition{0.0f, 0.0f};
        EventCallback m_EventCallback;
        NativeEventCallback m_NativeEventCallback;
    };
}
