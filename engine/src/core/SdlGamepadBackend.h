#pragma once

#include <quasar/core/GamepadCodes.h>
#include <quasar/core/Gamepads.h>

#include <SDL3/SDL_joystick.h>

#include <array>
#include <vector>

struct SDL_Gamepad;

namespace Quasar
{
    class SdlGamepadBackend final : public GamepadBackend
    {
    public:
        SdlGamepadBackend();
        ~SdlGamepadBackend() override;

        SdlGamepadBackend(const SdlGamepadBackend&) = delete;
        SdlGamepadBackend& operator=(const SdlGamepadBackend&) = delete;
        SdlGamepadBackend(SdlGamepadBackend&&) = delete;
        SdlGamepadBackend& operator=(SdlGamepadBackend&&) = delete;

        GamepadReadings Read() override;

        bool Rumble(GamepadId gamepad, const GamepadRumble& rumble) override;

    private:
        void CloseDisconnected(std::array<bool, MaxGamepads>& freedNow);
        void OpenConnected(const std::array<bool, MaxGamepads>& freedNow);

        bool m_Started = false;
        std::array<SDL_Gamepad*, MaxGamepads> m_Gamepads{};
        std::vector<SDL_JoystickID> m_Failed;
    };
}
