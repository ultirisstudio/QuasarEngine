#include <quasar/core/GamepadCodes.h>

#include <GLFW/glfw3.h>
#include <doctest/doctest.h>

namespace
{
    using Quasar::GamepadAxis;
    using Quasar::GamepadButton;
    using Quasar::GamepadId;

    static_assert(GamepadButton::A == static_cast<GamepadButton>(GLFW_GAMEPAD_BUTTON_A));
    static_assert(GamepadButton::B == static_cast<GamepadButton>(GLFW_GAMEPAD_BUTTON_B));
    static_assert(GamepadButton::X == static_cast<GamepadButton>(GLFW_GAMEPAD_BUTTON_X));
    static_assert(GamepadButton::Y == static_cast<GamepadButton>(GLFW_GAMEPAD_BUTTON_Y));
    static_assert(GamepadButton::LeftBumper == static_cast<GamepadButton>(GLFW_GAMEPAD_BUTTON_LEFT_BUMPER));
    static_assert(GamepadButton::RightBumper == static_cast<GamepadButton>(GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER));
    static_assert(GamepadButton::Back == static_cast<GamepadButton>(GLFW_GAMEPAD_BUTTON_BACK));
    static_assert(GamepadButton::Start == static_cast<GamepadButton>(GLFW_GAMEPAD_BUTTON_START));
    static_assert(GamepadButton::Guide == static_cast<GamepadButton>(GLFW_GAMEPAD_BUTTON_GUIDE));
    static_assert(GamepadButton::LeftThumb == static_cast<GamepadButton>(GLFW_GAMEPAD_BUTTON_LEFT_THUMB));
    static_assert(GamepadButton::RightThumb == static_cast<GamepadButton>(GLFW_GAMEPAD_BUTTON_RIGHT_THUMB));
    static_assert(GamepadButton::DPadUp == static_cast<GamepadButton>(GLFW_GAMEPAD_BUTTON_DPAD_UP));
    static_assert(GamepadButton::DPadRight == static_cast<GamepadButton>(GLFW_GAMEPAD_BUTTON_DPAD_RIGHT));
    static_assert(GamepadButton::DPadDown == static_cast<GamepadButton>(GLFW_GAMEPAD_BUTTON_DPAD_DOWN));
    static_assert(GamepadButton::DPadLeft == static_cast<GamepadButton>(GLFW_GAMEPAD_BUTTON_DPAD_LEFT));
    static_assert(Quasar::GamepadButtonCount == GLFW_GAMEPAD_BUTTON_LAST + 1);

    static_assert(GamepadAxis::LeftX == static_cast<GamepadAxis>(GLFW_GAMEPAD_AXIS_LEFT_X));
    static_assert(GamepadAxis::LeftY == static_cast<GamepadAxis>(GLFW_GAMEPAD_AXIS_LEFT_Y));
    static_assert(GamepadAxis::RightX == static_cast<GamepadAxis>(GLFW_GAMEPAD_AXIS_RIGHT_X));
    static_assert(GamepadAxis::RightY == static_cast<GamepadAxis>(GLFW_GAMEPAD_AXIS_RIGHT_Y));
    static_assert(GamepadAxis::LeftTrigger == static_cast<GamepadAxis>(GLFW_GAMEPAD_AXIS_LEFT_TRIGGER));
    static_assert(GamepadAxis::RightTrigger == static_cast<GamepadAxis>(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER));
    static_assert(Quasar::GamepadAxisCount == GLFW_GAMEPAD_AXIS_LAST + 1);

    static_assert(Quasar::MaxGamepads == GLFW_JOYSTICK_LAST + 1);
}

TEST_CASE("A GLFW joystick id converts to a gamepad id with a cast")
{
    const int glfwJoystick = GLFW_JOYSTICK_3;

    CHECK(static_cast<GamepadId>(glfwJoystick) == GamepadId{2});
}