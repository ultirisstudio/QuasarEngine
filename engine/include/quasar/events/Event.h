#pragma once

/// \file
/// \brief Base class of all events, with their types and categories.
/// \author Kurogenshi
/// \date 2026-10-23

#include <quasar/core/Base.h>

#include <cstdint>
#include <string>
#include <type_traits>

namespace Quasar
{
    /// @brief Type of an event. Each event class has exactly one type.
    enum class EventType : std::uint8_t
    {
        WindowClose,         ///< The user asked to close the window.
        WindowResize,        ///< The size of the window changed.
        WindowFocus,         ///< The window gained or lost the focus.
        KeyPressed,          ///< A key was pressed, or is repeating while held down.
        KeyReleased,         ///< A key was released.
        KeyTyped,            ///< A character was typed, for text input.
        MouseButtonPressed,  ///< A mouse button was pressed.
        MouseButtonReleased, ///< A mouse button was released.
        MouseMoved,          ///< The cursor moved.
        MouseScrolled        ///< The mouse wheel or the touchpad scrolled.
    };

    /// @brief Categories of an event. An event can belong to several categories, combined with the `|` operator.
    enum class EventCategory : std::uint32_t
    {
        None = 0,          ///< No category.
        Window = Bit(0),   ///< Events about the window.
        Input = Bit(1),    ///< Events from an input device, keyboard or mouse.
        Keyboard = Bit(2), ///< Events from the keyboard.
        Mouse = Bit(3)     ///< Events from the mouse.
    };

    /// @brief Combines two sets of categories.
    constexpr EventCategory operator|(EventCategory left, EventCategory right)
    {
        using Underlying = std::underlying_type_t<EventCategory>;
        return static_cast<EventCategory>(static_cast<Underlying>(left) | static_cast<Underlying>(right));
    }

    /// @brief Keeps only the categories found in both sets.
    constexpr EventCategory operator&(EventCategory left, EventCategory right)
    {
        using Underlying = std::underlying_type_t<EventCategory>;
        return static_cast<EventCategory>(static_cast<Underlying>(left) & static_cast<Underlying>(right));
    }

    /// @brief Returns the name of an event type, for example "KeyPressed".
    constexpr const char* GetEventTypeName(EventType type)
    {
        switch (type)
        {
            case EventType::WindowClose:
                return "WindowClose";
            case EventType::WindowResize:
                return "WindowResize";
            case EventType::WindowFocus:
                return "WindowFocus";
            case EventType::KeyPressed:
                return "KeyPressed";
            case EventType::KeyReleased:
                return "KeyReleased";
            case EventType::KeyTyped:
                return "KeyTyped";
            case EventType::MouseButtonPressed:
                return "MouseButtonPressed";
            case EventType::MouseButtonReleased:
                return "MouseButtonReleased";
            case EventType::MouseMoved:
                return "MouseMoved";
            case EventType::MouseScrolled:
                return "MouseScrolled";
        }
        return "Unknown";
    }

    /// @brief Base class of all events.
    ///
    /// An event is a small value that describes something that happened, like a key press or a window resize. It is
    /// sent to the application, which can mark it as handled so that it is not processed any further.
    ///
    /// Event classes do not derive from Event directly but from EventOf, which gives them their type and categories.
    /// An event can be written to the log as is: `QUASAR_ENGINE_TRACE("{}", event)`.
    class Event
    {
    public:
        virtual ~Event() = default;

        /// @brief Returns the type of the event.
        virtual EventType GetType() const = 0;

        /// @brief Returns all the categories of the event.
        virtual EventCategory GetCategories() const = 0;

        /// @brief Returns a readable description of the event, for debugging.
        ///
        /// By default it is the name of the event type. Event classes with data add it, for example
        /// "WindowResize: 1280x720".
        virtual std::string ToString() const { return GetName(); }

        /// @brief Returns the name of the type of the event, for example "KeyPressed".
        const char* GetName() const { return GetEventTypeName(GetType()); }

        /// @brief Returns true if the event belongs to the given category.
        bool IsInCategory(EventCategory category) const { return (GetCategories() & category) != EventCategory::None; }

        /// @brief Returns true if the event was handled.
        bool IsHandled() const { return m_Handled; }

        /// @brief Marks the event as handled, or not handled.
        void SetHandled(bool handled) { m_Handled = handled; }

    protected:
        /// @brief Creates an event that is not handled.
        ///
        /// The constructors and assignments are protected: only event classes can create and copy events. Copying
        /// an Event on its own would lose the data of the event class.
        Event() = default;

        /// \cond
        Event(const Event&) = default;
        Event& operator=(const Event&) = default;
        Event(Event&&) = default;
        Event& operator=(Event&&) = default;
        /// \endcond

    private:
        bool m_Handled = false;
    };

    /// @brief Base class of each event class, which gives the event its type and its categories.
    ///
    /// For example: `class KeyPressedEvent : public EventOf<EventType::KeyPressed, EventCategory::Input |
    /// EventCategory::Keyboard>`. GetStaticType() gives the type without an event object, which lets the code check
    /// the type of an event before converting it.
    template <EventType Type, EventCategory Categories>
    class EventOf : public Event
    {
    public:
        /// @brief Returns the type of this event class.
        static constexpr EventType GetStaticType() { return Type; }

        /// @brief Returns the type of the event.
        EventType GetType() const override { return Type; }

        /// @brief Returns all the categories of the event.
        EventCategory GetCategories() const override { return Categories; }
    };

    /// @brief Lets the log macros print an event with `{}`, using Event::ToString().
    inline std::string format_as(const Event& event)
    {
        return event.ToString();
    }
}