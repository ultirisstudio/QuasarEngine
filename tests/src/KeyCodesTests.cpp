#include <quasar/core/KeyCodes.h>

#include <GLFW/glfw3.h>
#include <doctest/doctest.h>

namespace
{
    using Quasar::Key;

    static_assert(Key::Space == static_cast<Key>(GLFW_KEY_SPACE));
    static_assert(Key::Apostrophe == static_cast<Key>(GLFW_KEY_APOSTROPHE));
    static_assert(Key::Comma == static_cast<Key>(GLFW_KEY_COMMA));
    static_assert(Key::Minus == static_cast<Key>(GLFW_KEY_MINUS));
    static_assert(Key::Period == static_cast<Key>(GLFW_KEY_PERIOD));
    static_assert(Key::Slash == static_cast<Key>(GLFW_KEY_SLASH));
    static_assert(Key::Digit0 == static_cast<Key>(GLFW_KEY_0));
    static_assert(Key::Digit1 == static_cast<Key>(GLFW_KEY_1));
    static_assert(Key::Digit2 == static_cast<Key>(GLFW_KEY_2));
    static_assert(Key::Digit3 == static_cast<Key>(GLFW_KEY_3));
    static_assert(Key::Digit4 == static_cast<Key>(GLFW_KEY_4));
    static_assert(Key::Digit5 == static_cast<Key>(GLFW_KEY_5));
    static_assert(Key::Digit6 == static_cast<Key>(GLFW_KEY_6));
    static_assert(Key::Digit7 == static_cast<Key>(GLFW_KEY_7));
    static_assert(Key::Digit8 == static_cast<Key>(GLFW_KEY_8));
    static_assert(Key::Digit9 == static_cast<Key>(GLFW_KEY_9));
    static_assert(Key::Semicolon == static_cast<Key>(GLFW_KEY_SEMICOLON));
    static_assert(Key::Equal == static_cast<Key>(GLFW_KEY_EQUAL));
    static_assert(Key::A == static_cast<Key>(GLFW_KEY_A));
    static_assert(Key::B == static_cast<Key>(GLFW_KEY_B));
    static_assert(Key::C == static_cast<Key>(GLFW_KEY_C));
    static_assert(Key::D == static_cast<Key>(GLFW_KEY_D));
    static_assert(Key::E == static_cast<Key>(GLFW_KEY_E));
    static_assert(Key::F == static_cast<Key>(GLFW_KEY_F));
    static_assert(Key::G == static_cast<Key>(GLFW_KEY_G));
    static_assert(Key::H == static_cast<Key>(GLFW_KEY_H));
    static_assert(Key::I == static_cast<Key>(GLFW_KEY_I));
    static_assert(Key::J == static_cast<Key>(GLFW_KEY_J));
    static_assert(Key::K == static_cast<Key>(GLFW_KEY_K));
    static_assert(Key::L == static_cast<Key>(GLFW_KEY_L));
    static_assert(Key::M == static_cast<Key>(GLFW_KEY_M));
    static_assert(Key::N == static_cast<Key>(GLFW_KEY_N));
    static_assert(Key::O == static_cast<Key>(GLFW_KEY_O));
    static_assert(Key::P == static_cast<Key>(GLFW_KEY_P));
    static_assert(Key::Q == static_cast<Key>(GLFW_KEY_Q));
    static_assert(Key::R == static_cast<Key>(GLFW_KEY_R));
    static_assert(Key::S == static_cast<Key>(GLFW_KEY_S));
    static_assert(Key::T == static_cast<Key>(GLFW_KEY_T));
    static_assert(Key::U == static_cast<Key>(GLFW_KEY_U));
    static_assert(Key::V == static_cast<Key>(GLFW_KEY_V));
    static_assert(Key::W == static_cast<Key>(GLFW_KEY_W));
    static_assert(Key::X == static_cast<Key>(GLFW_KEY_X));
    static_assert(Key::Y == static_cast<Key>(GLFW_KEY_Y));
    static_assert(Key::Z == static_cast<Key>(GLFW_KEY_Z));
    static_assert(Key::LeftBracket == static_cast<Key>(GLFW_KEY_LEFT_BRACKET));
    static_assert(Key::Backslash == static_cast<Key>(GLFW_KEY_BACKSLASH));
    static_assert(Key::RightBracket == static_cast<Key>(GLFW_KEY_RIGHT_BRACKET));
    static_assert(Key::GraveAccent == static_cast<Key>(GLFW_KEY_GRAVE_ACCENT));
    static_assert(Key::World1 == static_cast<Key>(GLFW_KEY_WORLD_1));
    static_assert(Key::World2 == static_cast<Key>(GLFW_KEY_WORLD_2));
    static_assert(Key::Escape == static_cast<Key>(GLFW_KEY_ESCAPE));
    static_assert(Key::Enter == static_cast<Key>(GLFW_KEY_ENTER));
    static_assert(Key::Tab == static_cast<Key>(GLFW_KEY_TAB));
    static_assert(Key::Backspace == static_cast<Key>(GLFW_KEY_BACKSPACE));
    static_assert(Key::Insert == static_cast<Key>(GLFW_KEY_INSERT));
    static_assert(Key::Delete == static_cast<Key>(GLFW_KEY_DELETE));
    static_assert(Key::Right == static_cast<Key>(GLFW_KEY_RIGHT));
    static_assert(Key::Left == static_cast<Key>(GLFW_KEY_LEFT));
    static_assert(Key::Down == static_cast<Key>(GLFW_KEY_DOWN));
    static_assert(Key::Up == static_cast<Key>(GLFW_KEY_UP));
    static_assert(Key::PageUp == static_cast<Key>(GLFW_KEY_PAGE_UP));
    static_assert(Key::PageDown == static_cast<Key>(GLFW_KEY_PAGE_DOWN));
    static_assert(Key::Home == static_cast<Key>(GLFW_KEY_HOME));
    static_assert(Key::End == static_cast<Key>(GLFW_KEY_END));
    static_assert(Key::CapsLock == static_cast<Key>(GLFW_KEY_CAPS_LOCK));
    static_assert(Key::ScrollLock == static_cast<Key>(GLFW_KEY_SCROLL_LOCK));
    static_assert(Key::NumLock == static_cast<Key>(GLFW_KEY_NUM_LOCK));
    static_assert(Key::PrintScreen == static_cast<Key>(GLFW_KEY_PRINT_SCREEN));
    static_assert(Key::Pause == static_cast<Key>(GLFW_KEY_PAUSE));
    static_assert(Key::F1 == static_cast<Key>(GLFW_KEY_F1));
    static_assert(Key::F2 == static_cast<Key>(GLFW_KEY_F2));
    static_assert(Key::F3 == static_cast<Key>(GLFW_KEY_F3));
    static_assert(Key::F4 == static_cast<Key>(GLFW_KEY_F4));
    static_assert(Key::F5 == static_cast<Key>(GLFW_KEY_F5));
    static_assert(Key::F6 == static_cast<Key>(GLFW_KEY_F6));
    static_assert(Key::F7 == static_cast<Key>(GLFW_KEY_F7));
    static_assert(Key::F8 == static_cast<Key>(GLFW_KEY_F8));
    static_assert(Key::F9 == static_cast<Key>(GLFW_KEY_F9));
    static_assert(Key::F10 == static_cast<Key>(GLFW_KEY_F10));
    static_assert(Key::F11 == static_cast<Key>(GLFW_KEY_F11));
    static_assert(Key::F12 == static_cast<Key>(GLFW_KEY_F12));
    static_assert(Key::F13 == static_cast<Key>(GLFW_KEY_F13));
    static_assert(Key::F14 == static_cast<Key>(GLFW_KEY_F14));
    static_assert(Key::F15 == static_cast<Key>(GLFW_KEY_F15));
    static_assert(Key::F16 == static_cast<Key>(GLFW_KEY_F16));
    static_assert(Key::F17 == static_cast<Key>(GLFW_KEY_F17));
    static_assert(Key::F18 == static_cast<Key>(GLFW_KEY_F18));
    static_assert(Key::F19 == static_cast<Key>(GLFW_KEY_F19));
    static_assert(Key::F20 == static_cast<Key>(GLFW_KEY_F20));
    static_assert(Key::F21 == static_cast<Key>(GLFW_KEY_F21));
    static_assert(Key::F22 == static_cast<Key>(GLFW_KEY_F22));
    static_assert(Key::F23 == static_cast<Key>(GLFW_KEY_F23));
    static_assert(Key::F24 == static_cast<Key>(GLFW_KEY_F24));
    static_assert(Key::F25 == static_cast<Key>(GLFW_KEY_F25));
    static_assert(Key::Keypad0 == static_cast<Key>(GLFW_KEY_KP_0));
    static_assert(Key::Keypad1 == static_cast<Key>(GLFW_KEY_KP_1));
    static_assert(Key::Keypad2 == static_cast<Key>(GLFW_KEY_KP_2));
    static_assert(Key::Keypad3 == static_cast<Key>(GLFW_KEY_KP_3));
    static_assert(Key::Keypad4 == static_cast<Key>(GLFW_KEY_KP_4));
    static_assert(Key::Keypad5 == static_cast<Key>(GLFW_KEY_KP_5));
    static_assert(Key::Keypad6 == static_cast<Key>(GLFW_KEY_KP_6));
    static_assert(Key::Keypad7 == static_cast<Key>(GLFW_KEY_KP_7));
    static_assert(Key::Keypad8 == static_cast<Key>(GLFW_KEY_KP_8));
    static_assert(Key::Keypad9 == static_cast<Key>(GLFW_KEY_KP_9));
    static_assert(Key::KeypadDecimal == static_cast<Key>(GLFW_KEY_KP_DECIMAL));
    static_assert(Key::KeypadDivide == static_cast<Key>(GLFW_KEY_KP_DIVIDE));
    static_assert(Key::KeypadMultiply == static_cast<Key>(GLFW_KEY_KP_MULTIPLY));
    static_assert(Key::KeypadSubtract == static_cast<Key>(GLFW_KEY_KP_SUBTRACT));
    static_assert(Key::KeypadAdd == static_cast<Key>(GLFW_KEY_KP_ADD));
    static_assert(Key::KeypadEnter == static_cast<Key>(GLFW_KEY_KP_ENTER));
    static_assert(Key::KeypadEqual == static_cast<Key>(GLFW_KEY_KP_EQUAL));
    static_assert(Key::LeftShift == static_cast<Key>(GLFW_KEY_LEFT_SHIFT));
    static_assert(Key::LeftControl == static_cast<Key>(GLFW_KEY_LEFT_CONTROL));
    static_assert(Key::LeftAlt == static_cast<Key>(GLFW_KEY_LEFT_ALT));
    static_assert(Key::LeftSuper == static_cast<Key>(GLFW_KEY_LEFT_SUPER));
    static_assert(Key::RightShift == static_cast<Key>(GLFW_KEY_RIGHT_SHIFT));
    static_assert(Key::RightControl == static_cast<Key>(GLFW_KEY_RIGHT_CONTROL));
    static_assert(Key::RightAlt == static_cast<Key>(GLFW_KEY_RIGHT_ALT));
    static_assert(Key::RightSuper == static_cast<Key>(GLFW_KEY_RIGHT_SUPER));
    static_assert(Key::Menu == static_cast<Key>(GLFW_KEY_MENU));
}

TEST_CASE("A GLFW key converts to a key code with a cast")
{
    const int glfwKey = GLFW_KEY_W;

    CHECK(static_cast<Key>(glfwKey) == Key::W);
}
