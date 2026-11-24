#pragma once

#include <EditorPanel.h>

#include <quasar/input/GamepadCodes.h>
#include <quasar/input/GamepadState.h>
#include <quasar/input/Gamepads.h>

#include <optional>

namespace Quasar::Editor
{
    struct EditorContext;

    class GamepadPanel final : public EditorPanel
    {
    public:
        GamepadPanel(EditorContext& context, Gamepads& gamepads);

    protected:
        void OnImGuiRender() override;

    private:
        std::optional<GamepadId> SelectGamepad();
        void DrawDetails(GamepadId gamepad, const GamepadState& state);

        Gamepads* m_Gamepads;
        GamepadId m_Selected{};
        bool m_RumbleFailed = false;
    };
}
