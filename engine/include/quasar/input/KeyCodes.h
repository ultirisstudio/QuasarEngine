#pragma once

/// \file
/// \brief Codes of the keyboard keys, independent from SDL.
/// \author Kurogenshi
/// \date 2026-10-23

#include <cstddef>
#include <cstdint>

namespace Quasar
{
    /// @brief Number of possible key codes. Every Key value is lower.
    inline constexpr std::size_t KeyCount = 512;

    /// @brief Physical key of the keyboard.
    ///
    /// A key code identifies the position of a key on the keyboard, not the character printed on it. The names
    /// follow the US QWERTY layout: Key::W is the key at the position of W on a QWERTY keyboard, which is Z on a
    /// French AZERTY keyboard. Movement bound to W, A, S and D is therefore at the same place on every layout.
    /// On AZERTY, the digits of the top row are typed with Shift, but their key codes do not change.
    ///
    /// The values are the same as the SDL scancodes, so an SDL scancode converts with `static_cast<Key>(scancode)`.
    // SDL scancodes go up to KeyCount - 1, so the keys without a name here still fit
    // NOLINTNEXTLINE(performance-enum-size)
    enum class Key : std::uint16_t
    {
        A = 4,  ///< A key on QWERTY, Q on AZERTY.
        B = 5,  ///< B key.
        C = 6,  ///< C key.
        D = 7,  ///< D key.
        E = 8,  ///< E key.
        F = 9,  ///< F key.
        G = 10, ///< G key.
        H = 11, ///< H key.
        I = 12, ///< I key.
        J = 13, ///< J key.
        K = 14, ///< K key.
        L = 15, ///< L key.
        M = 16, ///< M key on QWERTY, comma on AZERTY.
        N = 17, ///< N key.
        O = 18, ///< O key.
        P = 19, ///< P key.
        Q = 20, ///< Q key on QWERTY, A on AZERTY.
        R = 21, ///< R key.
        S = 22, ///< S key.
        T = 23, ///< T key.
        U = 24, ///< U key.
        V = 25, ///< V key.
        W = 26, ///< W key on QWERTY, Z on AZERTY.
        X = 27, ///< X key.
        Y = 28, ///< Y key.
        Z = 29, ///< Z key on QWERTY, W on AZERTY.

        Digit1 = 30, ///< 1 key of the top row.
        Digit2 = 31, ///< 2 key of the top row.
        Digit3 = 32, ///< 3 key of the top row.
        Digit4 = 33, ///< 4 key of the top row.
        Digit5 = 34, ///< 5 key of the top row.
        Digit6 = 35, ///< 6 key of the top row.
        Digit7 = 36, ///< 7 key of the top row.
        Digit8 = 37, ///< 8 key of the top row.
        Digit9 = 38, ///< 9 key of the top row.
        Digit0 = 39, ///< 0 key of the top row.

        Enter = 40,        ///< Enter key of the main keyboard.
        Escape = 41,       ///< Escape key.
        Backspace = 42,    ///< Backspace key.
        Tab = 43,          ///< Tab key.
        Space = 44,        ///< Space bar.
        Minus = 45,        ///< Minus key on QWERTY, closing parenthesis on AZERTY.
        Equal = 46,        ///< Equal key.
        LeftBracket = 47,  ///< Left bracket key on QWERTY, circumflex accent on AZERTY.
        RightBracket = 48, ///< Right bracket key on QWERTY, dollar sign on AZERTY.
        Backslash = 49,    ///< Backslash key on QWERTY, asterisk on AZERTY.
        Semicolon = 51,    ///< Semicolon key on QWERTY, M on AZERTY.
        Apostrophe = 52,   ///< Apostrophe key on QWERTY, u with grave accent on AZERTY.
        GraveAccent = 53,  ///< Key on the left of 1 in the top row.
        Comma = 54,        ///< Comma key on QWERTY, semicolon on AZERTY.
        Period = 55,       ///< Period key on QWERTY, colon on AZERTY.
        Slash = 56,        ///< Slash key on QWERTY, exclamation mark on AZERTY.
        CapsLock = 57,     ///< Caps Lock key.

        F1 = 58,  ///< F1 key.
        F2 = 59,  ///< F2 key.
        F3 = 60,  ///< F3 key.
        F4 = 61,  ///< F4 key.
        F5 = 62,  ///< F5 key.
        F6 = 63,  ///< F6 key.
        F7 = 64,  ///< F7 key.
        F8 = 65,  ///< F8 key.
        F9 = 66,  ///< F9 key.
        F10 = 67, ///< F10 key.
        F11 = 68, ///< F11 key.
        F12 = 69, ///< F12 key.

        PrintScreen = 70, ///< Print Screen key.
        ScrollLock = 71,  ///< Scroll Lock key.
        Pause = 72,       ///< Pause key.
        Insert = 73,      ///< Insert key.
        Home = 74,        ///< Home key.
        PageUp = 75,      ///< Page Up key.
        Delete = 76,      ///< Delete key.
        End = 77,         ///< End key.
        PageDown = 78,    ///< Page Down key.
        Right = 79,       ///< Right arrow key.
        Left = 80,        ///< Left arrow key.
        Down = 81,        ///< Down arrow key.
        Up = 82,          ///< Up arrow key.

        NumLock = 83,        ///< Num Lock key.
        KeypadDivide = 84,   ///< Divide key of the numeric keypad.
        KeypadMultiply = 85, ///< Multiply key of the numeric keypad.
        KeypadSubtract = 86, ///< Subtract key of the numeric keypad.
        KeypadAdd = 87,      ///< Add key of the numeric keypad.
        KeypadEnter = 88,    ///< Enter key of the numeric keypad.
        Keypad1 = 89,        ///< 1 key of the numeric keypad.
        Keypad2 = 90,        ///< 2 key of the numeric keypad.
        Keypad3 = 91,        ///< 3 key of the numeric keypad.
        Keypad4 = 92,        ///< 4 key of the numeric keypad.
        Keypad5 = 93,        ///< 5 key of the numeric keypad.
        Keypad6 = 94,        ///< 6 key of the numeric keypad.
        Keypad7 = 95,        ///< 7 key of the numeric keypad.
        Keypad8 = 96,        ///< 8 key of the numeric keypad.
        Keypad9 = 97,        ///< 9 key of the numeric keypad.
        Keypad0 = 98,        ///< 0 key of the numeric keypad.
        KeypadDecimal = 99,  ///< Decimal point key of the numeric keypad.

        NonUsBackslash = 100, ///< Key between left Shift and Z on ISO keyboards, less-than sign on AZERTY.
        Menu = 101,           ///< Menu key, next to the right Windows key.
        KeypadEqual = 103,    ///< Equal key of the numeric keypad.

        F13 = 104, ///< F13 key.
        F14 = 105, ///< F14 key.
        F15 = 106, ///< F15 key.
        F16 = 107, ///< F16 key.
        F17 = 108, ///< F17 key.
        F18 = 109, ///< F18 key.
        F19 = 110, ///< F19 key.
        F20 = 111, ///< F20 key.
        F21 = 112, ///< F21 key.
        F22 = 113, ///< F22 key.
        F23 = 114, ///< F23 key.
        F24 = 115, ///< F24 key.

        LeftControl = 224,  ///< Left Control key.
        LeftShift = 225,    ///< Left Shift key.
        LeftAlt = 226,      ///< Left Alt key.
        LeftSuper = 227,    ///< Left Windows key, or left Super key on Linux.
        RightControl = 228, ///< Right Control key.
        RightShift = 229,   ///< Right Shift key.
        RightAlt = 230,     ///< Right Alt key, which is Alt Gr on AZERTY.
        RightSuper = 231    ///< Right Windows key, or right Super key on Linux.
    };
}
