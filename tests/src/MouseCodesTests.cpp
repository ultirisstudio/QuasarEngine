#include <quasar/core/MouseCodes.h>

#include <GLFW/glfw3.h>
#include <doctest/doctest.h>

namespace
{
    using Quasar::MouseButton;

    static_assert(MouseButton::Left == static_cast<MouseButton>(GLFW_MOUSE_BUTTON_LEFT));
    static_assert(MouseButton::Right == static_cast<MouseButton>(GLFW_MOUSE_BUTTON_RIGHT));
    static_assert(MouseButton::Middle == static_cast<MouseButton>(GLFW_MOUSE_BUTTON_MIDDLE));
    static_assert(MouseButton::Button3 == static_cast<MouseButton>(GLFW_MOUSE_BUTTON_4));
    static_assert(MouseButton::Button4 == static_cast<MouseButton>(GLFW_MOUSE_BUTTON_5));
    static_assert(MouseButton::Button5 == static_cast<MouseButton>(GLFW_MOUSE_BUTTON_6));
    static_assert(MouseButton::Button6 == static_cast<MouseButton>(GLFW_MOUSE_BUTTON_7));
    static_assert(MouseButton::Button7 == static_cast<MouseButton>(GLFW_MOUSE_BUTTON_8));
}

TEST_CASE("A GLFW mouse button converts to a mouse button code with a cast")
{
    const int glfwButton = GLFW_MOUSE_BUTTON_RIGHT;

    CHECK(static_cast<MouseButton>(glfwButton) == MouseButton::Right);
}
