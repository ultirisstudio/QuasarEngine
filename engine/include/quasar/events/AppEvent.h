#pragma once

/// \file
/// \brief Events sent by the operating system about the whole application: termination, low memory and background.
/// \author Kurogenshi
/// \date 2026-11-13

#include <quasar/events/Event.h>

#include <spdlog/fmt/fmt.h>

#include <string>

namespace Quasar
{
    /// @brief Sent when the operating system ends the application, for example when Android destroys it.
    ///
    /// The application stops its main loop at the end of the frame, after the layers received the event, so that they
    /// can save what must be kept. Only sent on Android.
    class AppTerminatingEvent final : public EventOf<EventType::AppTerminating, EventCategory::Application>
    {
    };

    /// @brief Sent when the operating system is low on memory.
    ///
    /// The layers can free what they can load again, such as caches. Only sent on Android.
    class AppLowMemoryEvent final : public EventOf<EventType::AppLowMemory, EventCategory::Application>
    {
    };

    /// @brief Sent when the application goes to the background, or comes back to the foreground.
    ///
    /// While the application is in the background, its layers are updated but not rendered. On Android, the
    /// application is paused right after this event, until it comes back: the layers must save what must be kept when
    /// they receive it, since Android can end a paused application without warning. Only sent on Android.
    class AppBackgroundEvent final : public EventOf<EventType::AppBackground, EventCategory::Application>
    {
    public:
        /// @brief Creates the event.
        /// @param background True if the application went to the background, false if it came back to the foreground.
        explicit AppBackgroundEvent(bool background) : m_Background(background) {}

        /// @brief Returns true if the application went to the background, false if it came back to the foreground.
        bool IsInBackground() const { return m_Background; }

        /// @brief Returns the event with the change, for example "AppBackground: background".
        std::string ToString() const override { return fmt::format("{}: {}", GetName(), m_Background ? "background" : "foreground"); }

    private:
        bool m_Background = false;
    };
}
