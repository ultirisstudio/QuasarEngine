#pragma once

#include <quasar/input/GamepadCodes.h>
#include <quasar/input/GamepadState.h>
#include <quasar/input/Gamepads.h>

#include <optional>

namespace Quasar::Sandbox
{
    class GamepadPanel
    {
    public:
        void Draw();

    private:
        std::optional<GamepadId> SelectGamepad(const Gamepads& gamepads);
        void DrawDetails(Gamepads& gamepads, GamepadId gamepad, const GamepadState& state);

        GamepadId m_Selected{};
        bool m_RumbleFailed = false;
    };
}
