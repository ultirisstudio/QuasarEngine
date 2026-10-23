#pragma once

/// \file
/// \brief Events sent by the window: close, resize and focus changes.
/// \author Kurogenshi
/// \date 2026-10-23

#include <quasar/events/Event.h>

#include <spdlog/fmt/fmt.h>

#include <cstdint>
#include <string>

namespace Quasar
{
    /// @brief Sent when the user asks to close the window, for example with the close button.
    class WindowCloseEvent final : public EventOf<EventType::WindowClose, EventCategory::Window>
    {
    };

    /// @brief Sent when the size of the drawing area of the window changes.
    ///
    /// The size is in pixels, like Window::GetWidth() and Window::GetHeight(). It can be 0x0, for example when the
    /// window is minimized on Windows.
    class WindowResizeEvent final : public EventOf<EventType::WindowResize, EventCategory::Window>
    {
    public:
        /// @brief Creates the event with the new size, in pixels.
        WindowResizeEvent(std::uint32_t width, std::uint32_t height) : m_Width(width), m_Height(height) {}

        /// @brief Returns the new width, in pixels.
        std::uint32_t GetWidth() const { return m_Width; }

        /// @brief Returns the new height, in pixels.
        std::uint32_t GetHeight() const { return m_Height; }

        /// @brief Returns the event with its size, for example "WindowResize: 1280x720".
        std::string ToString() const override { return fmt::format("{}: {}x{}", GetName(), m_Width, m_Height); }

    private:
        std::uint32_t m_Width = 0;
        std::uint32_t m_Height = 0;
    };

    /// @brief Sent when the window gains or loses the keyboard focus.
    class WindowFocusEvent final : public EventOf<EventType::WindowFocus, EventCategory::Window>
    {
    public:
        /// @brief Creates the event.
        /// @param focused True if the window gained the focus, false if it lost it.
        explicit WindowFocusEvent(bool focused) : m_Focused(focused) {}

        /// @brief Returns true if the window gained the focus, false if it lost it.
        bool IsFocused() const { return m_Focused; }

        /// @brief Returns the event with the change, for example "WindowFocus: gained".
        std::string ToString() const override
        {
            return fmt::format("{}: {}", GetName(), m_Focused ? "gained" : "lost");
        }

    private:
        bool m_Focused = false;
    };
}
