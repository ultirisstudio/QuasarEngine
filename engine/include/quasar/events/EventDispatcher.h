#pragma once

/// \file
/// \brief Calls the right handler for an event, depending on its type.
/// \author Kurogenshi
/// \date 2026-10-23

#include <quasar/events/Event.h>

#include <concepts>
#include <functional>
#include <utility>

namespace Quasar
{
    /// @brief An event class: a class derived from Event that gives its type with GetStaticType().
    ///
    /// Every class derived from EventOf satisfies this concept.
    template <typename T>
    concept EventClass = std::derived_from<T, Event> && requires {
        { T::GetStaticType() } -> std::same_as<EventType>;
    };

    /// @brief Calls a handler only when an event has the type that the handler expects.
    ///
    /// Create a dispatcher for the received event, then call Dispatch() once for each event class to handle:
    ///
    /// ```cpp
    /// void OnEvent(Event& event)
    /// {
    ///     EventDispatcher dispatcher(event);
    ///     dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& resize) { return OnResize(resize); });
    /// }
    /// ```
    ///
    /// Adding a new event class does not require any change here: Dispatch() only relies on GetStaticType().
    class EventDispatcher
    {
    public:
        /// @brief Creates a dispatcher for an event. The event must stay alive while the dispatcher is used.
        explicit EventDispatcher(Event& event) : m_Event(event) {}

        /// @brief Calls the handler if the event is of type T.
        ///
        /// When the handler returns true, the event is marked as handled. A handled event stays handled: a handler
        /// that returns false never clears the flag set by another one.
        /// @tparam T Event class expected by the handler.
        /// @param handler Function that takes a `T&` and returns true if it handled the event.
        /// @return True if the event is of type T and the handler was called.
        template <EventClass T, std::predicate<T&> Handler>
        bool Dispatch(Handler&& handler)
        {
            if (m_Event.GetType() != T::GetStaticType())
            {
                return false;
            }

            if (std::invoke(std::forward<Handler>(handler), static_cast<T&>(m_Event)))
            {
                m_Event.SetHandled(true);
            }
            return true;
        }

    private:
        Event& m_Event;
    };
}