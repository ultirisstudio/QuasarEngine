#include <quasar/core/Application.h>
#include <quasar/core/Input.h>

#include <quasar/core/Window.h>

namespace Quasar::Input
{
    bool IsKeyPressed(Key key)
    {
        return Application::Get().GetWindow().IsKeyDown(key);
    }

    bool IsMouseButtonPressed(MouseButton button)
    {
        return Application::Get().GetWindow().IsMouseButtonDown(button);
    }

    glm::vec2 GetMousePosition()
    {
        return Application::Get().GetWindow().GetMousePosition();
    }

    bool IsGamepadConnected(GamepadId gamepad)
    {
        return Application::Get().GetGamepads().IsConnected(gamepad);
    }

    bool IsGamepadButtonPressed(GamepadId gamepad, GamepadButton button)
    {
        return Application::Get().GetGamepads().IsButtonPressed(gamepad, button);
    }

    float GetGamepadAxis(GamepadId gamepad, GamepadAxis axis)
    {
        return Application::Get().GetGamepads().GetAxis(gamepad, axis);
    }

    GamepadFinger GetGamepadTouchpadFinger(GamepadId gamepad, std::size_t touchpad, std::size_t finger)
    {
        return Application::Get().GetGamepads().GetTouchpadFinger(gamepad, touchpad, finger);
    }
}
