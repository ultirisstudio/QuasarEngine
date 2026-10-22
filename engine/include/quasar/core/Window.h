#pragma once

/// \file
/// \brief Application window, created with GLFW.
/// \author Kurogenshi
/// \date 2026-10-23

#include <quasar/core/Base.h>
#include <quasar/core/KeyCodes.h>

#include <cstdint>
#include <string>

struct GLFWwindow;

namespace Quasar
{
    /// @brief Settings used to create a window.
    ///
    /// Only the fields that differ from the defaults need a value: `Window::Create({.Title = "Editor"})`.
    struct WindowSpecification
    {
        std::string Title = "QuasarEngine"; ///< Title of the window.
        std::uint32_t Width = 1280;         ///< Width of the window when it is created.
        std::uint32_t Height = 720;         ///< Height of the window when it is created.
        bool VSync = true;                  ///< Whether vertical synchronization is enabled when it is created.
    };

    /// @brief Application window.
    ///
    /// A window is created with Create() and destroyed with its Scope. GLFW is initialized with the first window and
    /// shut down with the last one.
    class Window
    {
    public:
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        /// @brief Creates a window and shows it.
        /// @param specification Title, size and settings of the window.
        /// @return The new window, or `nullptr` if it cannot be created. The reason is logged as an engine error.
        [[nodiscard]] static Scope<Window> Create(const WindowSpecification& specification);

        /// @brief Shows the rendered image and processes the messages of the operating system.
        ///
        /// Call it once per frame, after rendering.
        void Update();

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

        /// @brief Returns the GLFW window, for the code that needs to call GLFW directly.
        GLFWwindow* GetNativeWindow() const;

        /// @brief Returns the name of a key in the current keyboard layout, for example "z" for Key::W on AZERTY.
        /// @return The name of the key, or an empty string for keys that do not print a character, like Escape.
        std::string GetKeyName(Key key) const;

    private:
        Window(GLFWwindow* handle, const WindowSpecification& specification);

        GLFWwindow* m_Handle = nullptr;
        std::string m_Title;
        bool m_VSync = false;
    };
}