#pragma once

#include <quasar/core/GamepadCodes.h>
#include <quasar/core/Gamepads.h>

#include <SDL3/SDL_joystick.h>

#include <array>
#include <vector>

struct SDL_Gamepad;

namespace Quasar
{
    class SdlGamepadReader
    {
    public:
        SdlGamepadReader();
        ~SdlGamepadReader();

        SdlGamepadReader(const SdlGamepadReader&) = delete;
        SdlGamepadReader& operator=(const SdlGamepadReader&) = delete;
        SdlGamepadReader(SdlGamepadReader&&) = delete;
        SdlGamepadReader& operator=(SdlGamepadReader&&) = delete;

        GamepadReadings Read();

    private:
        void CloseDisconnected(std::array<bool, MaxGamepads>& freedNow);
        void OpenConnected(const std::array<bool, MaxGamepads>& freedNow);

        bool m_Started = false;
        std::array<SDL_Gamepad*, MaxGamepads> m_Gamepads{};
        std::vector<SDL_JoystickID> m_Failed;
    };
}
