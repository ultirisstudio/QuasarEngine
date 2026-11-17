#pragma once

/// \file
/// \brief Events sent by a touch screen: fingers that touch it, move on it and leave it.
/// \author Kurogenshi
/// \date 2026-11-14

#include <quasar/events/Event.h>

#include <glm/vec2.hpp>
#include <spdlog/fmt/fmt.h>

#include <cstdint>
#include <string>

namespace Quasar
{
    /// @brief Id of a finger on a touch screen. A finger keeps its id from the moment it touches the screen until it
    /// leaves it, and several fingers on the screen at the same time have different ids.
    using FingerId = std::uint64_t;

    /// @brief Base class of the touch events, which carry the finger, its position and its pressure.
    ///
    /// The position is in screen coordinates, from the top-left corner of the drawing area, like the position of the
    /// mouse. A pen on the screen sends touch events too.
    template <EventType Type>
    class TouchEventOf : public EventOf<Type, EventCategory::Input | EventCategory::Touch>
    {
    public:
        /// @brief Creates the event for a finger on the screen.
        /// @param finger The id of the finger.
        /// @param position The position of the finger, in screen coordinates.
        /// @param pressure The pressure of the finger, from 0 to 1.
        TouchEventOf(FingerId finger, glm::vec2 position, float pressure) : m_Finger(finger), m_Position(position), m_Pressure(pressure) {}

        /// @brief Returns the id of the finger.
        FingerId GetFinger() const { return m_Finger; }

        /// @brief Returns the position of the finger, in screen coordinates, from the top-left corner of the drawing area.
        glm::vec2 GetPosition() const { return m_Position; }

        /// @brief Returns the pressure of the finger, from 0 to 1.
        float GetPressure() const { return m_Pressure; }

        /// @brief Returns the event with the finger, the position and the pressure, for example
        /// "TouchMoved: 3, 640, 360.5, 1".
        std::string ToString() const override { return fmt::format("{}: {}, {}, {}, {}", this->GetName(), m_Finger, m_Position.x, m_Position.y, m_Pressure); }

    private:
        FingerId m_Finger = 0;
        glm::vec2 m_Position{0.0f, 0.0f};
        float m_Pressure = 0.0f;
    };

    /// @brief Sent when a finger touches the screen.
    class TouchDownEvent final : public TouchEventOf<EventType::TouchDown>
    {
    public:
        using TouchEventOf::TouchEventOf;
    };

    /// @brief Sent when a finger moves on the screen, or presses it harder or softer.
    class TouchMovedEvent final : public TouchEventOf<EventType::TouchMoved>
    {
    public:
        using TouchEventOf::TouchEventOf;
    };

    /// @brief Sent when a finger leaves the screen, at the last position where it was down.
    ///
    /// It is also sent when the system takes the finger, for example for a navigation gesture of Android.
    class TouchUpEvent final : public TouchEventOf<EventType::TouchUp>
    {
    public:
        using TouchEventOf::TouchEventOf;
    };
}
