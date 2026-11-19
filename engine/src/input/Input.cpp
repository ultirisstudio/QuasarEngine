#include <quasar/core/Application.h>
#include <quasar/input/Gamepads.h>
#include <quasar/input/Input.h>

#include <quasar/core/Window.h>
#include <quasar/imgui/ImGuiLayer.h>

namespace Quasar::Input
{
    namespace
    {
        bool IsMouseBlocked()
        {
            const ImGuiLayer* imgui = Application::Get().GetImGuiLayer();
            return imgui != nullptr && imgui->IsBlockingMouse();
        }

        bool IsKeyboardBlocked()
        {
            const ImGuiLayer* imgui = Application::Get().GetImGuiLayer();
            return imgui != nullptr && imgui->IsBlockingKeyboard();
        }
    }

    bool IsKeyPressed(Key key)
    {
        return !IsKeyboardBlocked() && Application::Get().GetWindow().IsKeyDown(key);
    }

    float GetKeyAxis(Key negative, Key positive)
    {
        return (IsKeyPressed(positive) ? 1.0f : 0.0f) - (IsKeyPressed(negative) ? 1.0f : 0.0f);
    }

    bool IsMouseButtonPressed(MouseButton button)
    {
        return !IsMouseBlocked() && Application::Get().GetWindow().IsMouseButtonDown(button);
    }

    glm::vec2 GetMousePosition()
    {
        return Application::Get().GetWindow().GetMousePosition();
    }

    glm::vec2 GetMouseDelta()
    {
        return IsMouseBlocked() ? glm::vec2(0.0f) : Application::Get().GetWindow().GetMouseDelta();
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
