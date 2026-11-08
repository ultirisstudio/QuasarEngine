#include <quasar/core/GamepadCodes.h>

#include <SDL3/SDL_gamepad.h>

namespace
{
    using Quasar::GamepadAxis;
    using Quasar::GamepadButton;
    using Quasar::GamepadType;

    static_assert(GamepadButton::A == static_cast<GamepadButton>(SDL_GAMEPAD_BUTTON_SOUTH));
    static_assert(GamepadButton::B == static_cast<GamepadButton>(SDL_GAMEPAD_BUTTON_EAST));
    static_assert(GamepadButton::X == static_cast<GamepadButton>(SDL_GAMEPAD_BUTTON_WEST));
    static_assert(GamepadButton::Y == static_cast<GamepadButton>(SDL_GAMEPAD_BUTTON_NORTH));
    static_assert(GamepadButton::Back == static_cast<GamepadButton>(SDL_GAMEPAD_BUTTON_BACK));
    static_assert(GamepadButton::Guide == static_cast<GamepadButton>(SDL_GAMEPAD_BUTTON_GUIDE));
    static_assert(GamepadButton::Start == static_cast<GamepadButton>(SDL_GAMEPAD_BUTTON_START));
    static_assert(GamepadButton::LeftThumb == static_cast<GamepadButton>(SDL_GAMEPAD_BUTTON_LEFT_STICK));
    static_assert(GamepadButton::RightThumb == static_cast<GamepadButton>(SDL_GAMEPAD_BUTTON_RIGHT_STICK));
    static_assert(GamepadButton::LeftBumper == static_cast<GamepadButton>(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER));
    static_assert(GamepadButton::RightBumper == static_cast<GamepadButton>(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER));
    static_assert(GamepadButton::DPadUp == static_cast<GamepadButton>(SDL_GAMEPAD_BUTTON_DPAD_UP));
    static_assert(GamepadButton::DPadDown == static_cast<GamepadButton>(SDL_GAMEPAD_BUTTON_DPAD_DOWN));
    static_assert(GamepadButton::DPadLeft == static_cast<GamepadButton>(SDL_GAMEPAD_BUTTON_DPAD_LEFT));
    static_assert(GamepadButton::DPadRight == static_cast<GamepadButton>(SDL_GAMEPAD_BUTTON_DPAD_RIGHT));
    
    static_assert(Quasar::GamepadButtonCount == SDL_GAMEPAD_BUTTON_DPAD_RIGHT + 1);

    static_assert(GamepadAxis::LeftX == static_cast<GamepadAxis>(SDL_GAMEPAD_AXIS_LEFTX));
    static_assert(GamepadAxis::LeftY == static_cast<GamepadAxis>(SDL_GAMEPAD_AXIS_LEFTY));
    static_assert(GamepadAxis::RightX == static_cast<GamepadAxis>(SDL_GAMEPAD_AXIS_RIGHTX));
    static_assert(GamepadAxis::RightY == static_cast<GamepadAxis>(SDL_GAMEPAD_AXIS_RIGHTY));
    static_assert(GamepadAxis::LeftTrigger == static_cast<GamepadAxis>(SDL_GAMEPAD_AXIS_LEFT_TRIGGER));
    static_assert(GamepadAxis::RightTrigger == static_cast<GamepadAxis>(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER));
    static_assert(Quasar::GamepadAxisCount == SDL_GAMEPAD_AXIS_COUNT);

    static_assert(GamepadType::Unknown == static_cast<GamepadType>(SDL_GAMEPAD_TYPE_UNKNOWN));
    static_assert(GamepadType::Standard == static_cast<GamepadType>(SDL_GAMEPAD_TYPE_STANDARD));
    static_assert(GamepadType::Xbox360 == static_cast<GamepadType>(SDL_GAMEPAD_TYPE_XBOX360));
    static_assert(GamepadType::XboxOne == static_cast<GamepadType>(SDL_GAMEPAD_TYPE_XBOXONE));
    static_assert(GamepadType::PlayStation3 == static_cast<GamepadType>(SDL_GAMEPAD_TYPE_PS3));
    static_assert(GamepadType::PlayStation4 == static_cast<GamepadType>(SDL_GAMEPAD_TYPE_PS4));
    static_assert(GamepadType::PlayStation5 == static_cast<GamepadType>(SDL_GAMEPAD_TYPE_PS5));
    static_assert(GamepadType::SwitchPro == static_cast<GamepadType>(SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO));
    static_assert(GamepadType::JoyConLeft == static_cast<GamepadType>(SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT));
    static_assert(GamepadType::JoyConRight == static_cast<GamepadType>(SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT));
    static_assert(GamepadType::JoyConPair == static_cast<GamepadType>(SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR));
    static_assert(GamepadType::GameCube == static_cast<GamepadType>(SDL_GAMEPAD_TYPE_GAMECUBE));
    static_assert(GamepadType::SteamController == static_cast<GamepadType>(SDL_GAMEPAD_TYPE_STEAM));
    
    static_assert(static_cast<int>(GamepadType::SteamController) + 1 == SDL_GAMEPAD_TYPE_COUNT);
}
