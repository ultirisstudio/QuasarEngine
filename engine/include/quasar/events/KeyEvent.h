#pragma once

/// \file
/// \brief Events sent by the keyboard: key presses, key releases and typed characters.
/// \author Kurogenshi
/// \date 2026-10-23

#include <quasar/core/KeyCodes.h>
#include <quasar/events/Event.h>

#include <spdlog/fmt/fmt.h>

#include <cstdint>
#include <string>

namespace Quasar
{
    /// @brief Sent when a key is pressed, and again at regular intervals while it is held down.
    ///
    /// Use it for actions and shortcuts. To read text typed by the user, use KeyTypedEvent instead.
    class KeyPressedEvent final : public EventOf<EventType::KeyPressed, EventCategory::Input | EventCategory::Keyboard>
    {
    public:
        /// @brief Creates the event.
        /// @param key The pressed key.
        /// @param repeat True if the event comes from the key being held down, false for the first press.
        explicit KeyPressedEvent(Key key, bool repeat = false) : m_Key(key), m_Repeat(repeat) {}

        /// @brief Returns the pressed key.
        Key GetKey() const { return m_Key; }

        /// @brief Returns true if the event comes from the key being held down, false for the first press.
        bool IsRepeat() const { return m_Repeat; }

        /// @brief Returns the event with its key code, for example "KeyPressed: 87 (repeat)".
        std::string ToString() const override
        {
            return fmt::format("{}: {}{}", GetName(), static_cast<std::uint16_t>(m_Key), m_Repeat ? " (repeat)" : "");
        }

    private:
        Key m_Key;
        bool m_Repeat = false;
    };

    /// @brief Sent when a key is released.
    class KeyReleasedEvent final
        : public EventOf<EventType::KeyReleased, EventCategory::Input | EventCategory::Keyboard>
    {
    public:
        /// @brief Creates the event for the released key.
        explicit KeyReleasedEvent(Key key) : m_Key(key) {}

        /// @brief Returns the released key.
        Key GetKey() const { return m_Key; }

        /// @brief Returns the event with its key code, for example "KeyReleased: 87".
        std::string ToString() const override
        {
            return fmt::format("{}: {}", GetName(), static_cast<std::uint16_t>(m_Key));
        }

    private:
        Key m_Key;
    };

    /// @brief Sent when the user types a character, for text input.
    ///
    /// A typed character is not the same thing as a key press: it depends on the keyboard layout, on Shift and on
    /// dead keys, and some characters need several keys. Use this event for text fields, and KeyPressedEvent for
    /// actions.
    class KeyTypedEvent final : public EventOf<EventType::KeyTyped, EventCategory::Input | EventCategory::Keyboard>
    {
    public:
        /// @brief Creates the event for the typed character, as a Unicode code point.
        explicit KeyTypedEvent(char32_t character) : m_Character(character) {}

        /// @brief Returns the typed character, as a Unicode code point.
        char32_t GetCharacter() const { return m_Character; }

        /// @brief Returns the event with its character, for example "KeyTyped: 'A' (U+0041)" or "KeyTyped: U+00E9".
        ///
        /// Only printable ASCII characters are shown as they are, so the log stays readable on every console.
        std::string ToString() const override
        {
            const auto codePoint = static_cast<std::uint32_t>(m_Character);
            if (codePoint >= 0x20 && codePoint <= 0x7E)
            {
                return fmt::format("{}: '{}' (U+{:04X})", GetName(), static_cast<char>(codePoint), codePoint);
            }
            return fmt::format("{}: U+{:04X}", GetName(), codePoint);
        }

    private:
        char32_t m_Character;
    };
}
