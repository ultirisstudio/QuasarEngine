#pragma once

/// \file
/// \brief Codes of the keyboard keys, independent from GLFW.
/// \author Kurogenshi
/// \date 2026-10-23

#include <cstdint>

namespace Quasar
{
    /// @brief Physical key of the keyboard.
    ///
    /// A key code identifies the position of a key on the keyboard, not the character printed on it. The names
    /// follow the US QWERTY layout: Key::W is the key at the position of W on a QWERTY keyboard, which is Z on a
    /// French AZERTY keyboard. Movement bound to W, A, S and D is therefore at the same place on every layout.
    /// On AZERTY, the digits of the top row are typed with Shift, but their key codes do not change.
    ///
    /// The values are the same as the GLFW key tokens, so a GLFW key converts with `static_cast<Key>(key)`.
    /// GLFW reports the keys it does not know as `GLFW_KEY_UNKNOWN` (-1), which has no Key value: ignore them.
    enum class Key : std::uint16_t
    {
        Space = 32,      ///< Space bar.
        Apostrophe = 39, ///< Apostrophe key on QWERTY.
        Comma = 44,      ///< Comma key on QWERTY, semicolon on AZERTY.
        Minus = 45,      ///< Minus key on QWERTY, closing parenthesis on AZERTY.
        Period = 46,     ///< Period key on QWERTY, colon on AZERTY.
        Slash = 47,      ///< Slash key on QWERTY, exclamation mark on AZERTY.

        Digit0 = 48, ///< 0 key of the top row.
        Digit1 = 49, ///< 1 key of the top row.
        Digit2 = 50, ///< 2 key of the top row.
        Digit3 = 51, ///< 3 key of the top row.
        Digit4 = 52, ///< 4 key of the top row.
        Digit5 = 53, ///< 5 key of the top row.
        Digit6 = 54, ///< 6 key of the top row.
        Digit7 = 55, ///< 7 key of the top row.
        Digit8 = 56, ///< 8 key of the top row.
        Digit9 = 57, ///< 9 key of the top row.

        Semicolon = 59, ///< Semicolon key on QWERTY, M on AZERTY.
        Equal = 61,     ///< Equal key.

        A = 65, ///< A key on QWERTY, Q on AZERTY.
        B = 66, ///< B key.
        C = 67, ///< C key.
        D = 68, ///< D key.
        E = 69, ///< E key.
        F = 70, ///< F key.
        G = 71, ///< G key.
        H = 72, ///< H key.
        I = 73, ///< I key.
        J = 74, ///< J key.
        K = 75, ///< K key.
        L = 76, ///< L key.
        M = 77, ///< M key on QWERTY, comma on AZERTY.
        N = 78, ///< N key.
        O = 79, ///< O key.
        P = 80, ///< P key.
        Q = 81, ///< Q key on QWERTY, A on AZERTY.
        R = 82, ///< R key.
        S = 83, ///< S key.
        T = 84, ///< T key.
        U = 85, ///< U key.
        V = 86, ///< V key.
        W = 87, ///< W key on QWERTY, Z on AZERTY.
        X = 88, ///< X key.
        Y = 89, ///< Y key.
        Z = 90, ///< Z key on QWERTY, W on AZERTY.

        LeftBracket = 91,  ///< Left bracket key on QWERTY, circumflex accent on AZERTY.
        Backslash = 92,    ///< Backslash key on QWERTY, asterisk on AZERTY.
        RightBracket = 93, ///< Right bracket key on QWERTY, dollar sign on AZERTY.
        GraveAccent = 96,  ///< Key on the left of 1 in the top row.

        World1 = 161, ///< Extra key found on some non-US keyboards.
        World2 = 162, ///< Extra key found on some non-US keyboards.

        Escape = 256,      ///< Escape key.
        Enter = 257,       ///< Enter key of the main keyboard.
        Tab = 258,         ///< Tab key.
        Backspace = 259,   ///< Backspace key.
        Insert = 260,      ///< Insert key.
        Delete = 261,      ///< Delete key.
        Right = 262,       ///< Right arrow key.
        Left = 263,        ///< Left arrow key.
        Down = 264,        ///< Down arrow key.
        Up = 265,          ///< Up arrow key.
        PageUp = 266,      ///< Page Up key.
        PageDown = 267,    ///< Page Down key.
        Home = 268,        ///< Home key.
        End = 269,         ///< End key.
        CapsLock = 280,    ///< Caps Lock key.
        ScrollLock = 281,  ///< Scroll Lock key.
        NumLock = 282,     ///< Num Lock key.
        PrintScreen = 283, ///< Print Screen key.
        Pause = 284,       ///< Pause key.

        F1 = 290,  ///< F1 key.
        F2 = 291,  ///< F2 key.
        F3 = 292,  ///< F3 key.
        F4 = 293,  ///< F4 key.
        F5 = 294,  ///< F5 key.
        F6 = 295,  ///< F6 key.
        F7 = 296,  ///< F7 key.
        F8 = 297,  ///< F8 key.
        F9 = 298,  ///< F9 key.
        F10 = 299, ///< F10 key.
        F11 = 300, ///< F11 key.
        F12 = 301, ///< F12 key.
        F13 = 302, ///< F13 key.
        F14 = 303, ///< F14 key.
        F15 = 304, ///< F15 key.
        F16 = 305, ///< F16 key.
        F17 = 306, ///< F17 key.
        F18 = 307, ///< F18 key.
        F19 = 308, ///< F19 key.
        F20 = 309, ///< F20 key.
        F21 = 310, ///< F21 key.
        F22 = 311, ///< F22 key.
        F23 = 312, ///< F23 key.
        F24 = 313, ///< F24 key.
        F25 = 314, ///< F25 key.

        Keypad0 = 320,        ///< 0 key of the numeric keypad.
        Keypad1 = 321,        ///< 1 key of the numeric keypad.
        Keypad2 = 322,        ///< 2 key of the numeric keypad.
        Keypad3 = 323,        ///< 3 key of the numeric keypad.
        Keypad4 = 324,        ///< 4 key of the numeric keypad.
        Keypad5 = 325,        ///< 5 key of the numeric keypad.
        Keypad6 = 326,        ///< 6 key of the numeric keypad.
        Keypad7 = 327,        ///< 7 key of the numeric keypad.
        Keypad8 = 328,        ///< 8 key of the numeric keypad.
        Keypad9 = 329,        ///< 9 key of the numeric keypad.
        KeypadDecimal = 330,  ///< Decimal point key of the numeric keypad.
        KeypadDivide = 331,   ///< Divide key of the numeric keypad.
        KeypadMultiply = 332, ///< Multiply key of the numeric keypad.
        KeypadSubtract = 333, ///< Subtract key of the numeric keypad.
        KeypadAdd = 334,      ///< Add key of the numeric keypad.
        KeypadEnter = 335,    ///< Enter key of the numeric keypad.
        KeypadEqual = 336,    ///< Equal key of the numeric keypad.

        LeftShift = 340,    ///< Left Shift key.
        LeftControl = 341,  ///< Left Control key.
        LeftAlt = 342,      ///< Left Alt key.
        LeftSuper = 343,    ///< Left Windows key, or left Super key on Linux.
        RightShift = 344,   ///< Right Shift key.
        RightControl = 345, ///< Right Control key.
        RightAlt = 346,     ///< Right Alt key, which is Alt Gr on AZERTY.
        RightSuper = 347,   ///< Right Windows key, or right Super key on Linux.
        Menu = 348          ///< Menu key, next to the right Windows key.
    };
}
