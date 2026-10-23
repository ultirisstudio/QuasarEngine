#pragma once

/// \file
/// \brief Events sent by the mouse: cursor moves, scrolling and button presses and releases.
/// \author Kurogenshi
/// \date 2026-10-23

#include <quasar/core/MouseCodes.h>
#include <quasar/events/Event.h>

#include <spdlog/fmt/fmt.h>

#include <string>

namespace Quasar
{
    /// @brief Sent when the cursor moves over the window.
    ///
    /// The position is in screen coordinates, from the top-left corner of the drawing area. On a scaled display,
    /// for example at 150%, screen coordinates are not pixels: the same position in pixels is larger.
    class MouseMovedEvent final : public EventOf<EventType::MouseMoved, EventCategory::Input | EventCategory::Mouse>
    {
    public:
        /// @brief Creates the event with the new position of the cursor, in screen coordinates.
        MouseMovedEvent(float x, float y) : m_X(x), m_Y(y) {}

        /// @brief Returns the horizontal position of the cursor, from the left edge of the drawing area.
        float GetX() const { return m_X; }

        /// @brief Returns the vertical position of the cursor, from the top edge of the drawing area.
        float GetY() const { return m_Y; }

        /// @brief Returns the event with the position, for example "MouseMoved: 640, 360.5".
        std::string ToString() const override { return fmt::format("{}: {}, {}", GetName(), m_X, m_Y); }

    private:
        float m_X = 0.0f;
        float m_Y = 0.0f;
    };

    /// @brief Sent when the mouse wheel or the touchpad scrolls.
    ///
    /// A standard mouse wheel only gives vertical offsets, of 1 per notch. Touchpads also give horizontal offsets,
    /// with fractional values.
    class MouseScrolledEvent final
        : public EventOf<EventType::MouseScrolled, EventCategory::Input | EventCategory::Mouse>
    {
    public:
        /// @brief Creates the event with the scroll offsets.
        MouseScrolledEvent(float xOffset, float yOffset) : m_XOffset(xOffset), m_YOffset(yOffset) {}

        /// @brief Returns the horizontal offset. It is positive when scrolling to the right.
        float GetXOffset() const { return m_XOffset; }

        /// @brief Returns the vertical offset. It is positive when the wheel turns away from the user.
        float GetYOffset() const { return m_YOffset; }

        /// @brief Returns the event with the offsets, for example "MouseScrolled: 0, 1".
        std::string ToString() const override { return fmt::format("{}: {}, {}", GetName(), m_XOffset, m_YOffset); }

    private:
        float m_XOffset = 0.0f;
        float m_YOffset = 0.0f;
    };

    /// @brief Sent when a mouse button is pressed.
    class MouseButtonPressedEvent final
        : public EventOf<EventType::MouseButtonPressed, EventCategory::Input | EventCategory::Mouse>
    {
    public:
        /// @brief Creates the event for the pressed button.
        explicit MouseButtonPressedEvent(MouseButton button) : m_Button(button) {}

        /// @brief Returns the pressed button.
        MouseButton GetButton() const { return m_Button; }

        /// @brief Returns the event with its button code, for example "MouseButtonPressed: 0" for the left button.
        std::string ToString() const override { return fmt::format("{}: {}", GetName(), static_cast<int>(m_Button)); }

    private:
        MouseButton m_Button;
    };

    /// @brief Sent when a mouse button is released.
    class MouseButtonReleasedEvent final
        : public EventOf<EventType::MouseButtonReleased, EventCategory::Input | EventCategory::Mouse>
    {
    public:
        /// @brief Creates the event for the released button.
        explicit MouseButtonReleasedEvent(MouseButton button) : m_Button(button) {}

        /// @brief Returns the released button.
        MouseButton GetButton() const { return m_Button; }

        /// @brief Returns the event with its button code, for example "MouseButtonReleased: 0" for the left button.
        std::string ToString() const override { return fmt::format("{}: {}", GetName(), static_cast<int>(m_Button)); }

    private:
        MouseButton m_Button;
    };
}