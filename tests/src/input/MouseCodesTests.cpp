#include <quasar/input/MouseCodes.h>

#include <SDL3/SDL_mouse.h>

namespace
{
    using Quasar::MouseButton;

    static_assert(MouseButton::Left == static_cast<MouseButton>(SDL_BUTTON_LEFT));
    static_assert(MouseButton::Middle == static_cast<MouseButton>(SDL_BUTTON_MIDDLE));
    static_assert(MouseButton::Right == static_cast<MouseButton>(SDL_BUTTON_RIGHT));
    static_assert(MouseButton::Back == static_cast<MouseButton>(SDL_BUTTON_X1));
    static_assert(MouseButton::Forward == static_cast<MouseButton>(SDL_BUTTON_X2));
}
