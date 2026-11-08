#include <core/SdlGamepadBackend.h>

#include <quasar/core/Log.h>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_stdinc.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace Quasar
{
    namespace
    {
        // SDL gives the stick axes from -32768 to 32767, and the triggers from 0 to 32767
        constexpr float AxisRange = 32767.0f;

        // SDL takes the strength of a rumble motor from 0 to 65535
        constexpr float RumbleRange = 65535.0f;

        Uint16 ToRumbleStrength(float strength)
        {
            return static_cast<Uint16>(std::lround(std::clamp(strength, 0.0f, 1.0f) * RumbleRange));
        }

        struct SdlFree
        {
            void operator()(SDL_JoystickID* ids) const { SDL_free(ids); }
        };

        GamepadReading ReadGamepad(SDL_Gamepad* gamepad)
        {
            GamepadReading reading;
            const char* name = SDL_GetGamepadName(gamepad);
            reading.Name = name != nullptr ? name : "";
            reading.Type = static_cast<GamepadType>(SDL_GetGamepadType(gamepad));

            for (std::size_t button = 0; button < GamepadButtonCount; ++button)
            {
                reading.State.Buttons.at(button) = SDL_GetGamepadButton(gamepad, static_cast<SDL_GamepadButton>(button));
            }
            for (std::size_t axis = 0; axis < GamepadAxisCount; ++axis)
            {
                const float value = static_cast<float>(SDL_GetGamepadAxis(gamepad, static_cast<SDL_GamepadAxis>(axis))) / AxisRange;
                reading.State.Axes.at(axis) = std::clamp(value, -1.0f, 1.0f);
            }
            return reading;
        }
    }

    SdlGamepadBackend::SdlGamepadBackend()
    {
        if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
        {
            QUASAR_ENGINE_ERROR("Cannot read the gamepads: {}", SDL_GetError());
            return;
        }
        m_Started = true;

        SDL_SetJoystickEventsEnabled(false);
        SDL_SetGamepadEventsEnabled(false);
    }

    SdlGamepadBackend::~SdlGamepadBackend()
    {
        for (SDL_Gamepad*& gamepad : m_Gamepads)
        {
            if (gamepad != nullptr)
            {
                SDL_CloseGamepad(gamepad);
                gamepad = nullptr;
            }
        }

        if (m_Started)
        {
            SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
        }
    }

    GamepadReadings SdlGamepadBackend::Read()
    {
        GamepadReadings readings{};
        if (!m_Started)
        {
            return readings;
        }

        SDL_UpdateGamepads();

        std::array<bool, MaxGamepads> freedNow{};
        CloseDisconnected(freedNow);
        OpenConnected(freedNow);

        for (std::size_t index = 0; index < MaxGamepads; ++index)
        {
            if (m_Gamepads.at(index) != nullptr)
            {
                readings.at(index) = ReadGamepad(m_Gamepads.at(index));
            }
        }
        return readings;
    }

    bool SdlGamepadBackend::Rumble(GamepadId gamepad, const GamepadRumble& rumble)
    {
        const auto index = static_cast<std::size_t>(gamepad);
        if (index >= MaxGamepads || m_Gamepads.at(index) == nullptr)
        {
            return false;
        }

        const bool stop = rumble.Duration.count() <= 0;
        const Uint16 low = stop ? 0 : ToRumbleStrength(rumble.LowFrequency);
        const Uint16 high = stop ? 0 : ToRumbleStrength(rumble.HighFrequency);
        const auto duration = static_cast<Uint32>(std::min<std::chrono::milliseconds::rep>(rumble.Duration.count(), SDL_MAX_UINT32));
        return SDL_RumbleGamepad(m_Gamepads.at(index), low, high, duration);
    }

    void SdlGamepadBackend::CloseDisconnected(std::array<bool, MaxGamepads>& freedNow)
    {
        for (std::size_t index = 0; index < MaxGamepads; ++index)
        {
            SDL_Gamepad*& gamepad = m_Gamepads.at(index);
            if (gamepad != nullptr && !SDL_GamepadConnected(gamepad))
            {
                SDL_CloseGamepad(gamepad);
                gamepad = nullptr;
                freedNow.at(index) = true;
            }
        }
    }

    void SdlGamepadBackend::OpenConnected(const std::array<bool, MaxGamepads>& freedNow)
    {
        int count = 0;
        const std::unique_ptr<SDL_JoystickID, SdlFree> ids(SDL_GetGamepads(&count));
        if (ids == nullptr)
        {
            return;
        }

        for (const SDL_JoystickID id : std::span(ids.get(), static_cast<std::size_t>(count)))
        {
            const bool alreadyOpen =
                std::ranges::any_of(m_Gamepads, [id](SDL_Gamepad* gamepad) { return gamepad != nullptr && SDL_GetGamepadID(gamepad) == id; });
            if (alreadyOpen)
            {
                continue;
            }

            std::size_t index = 0;
            while (index < MaxGamepads && (m_Gamepads.at(index) != nullptr || freedNow.at(index)))
            {
                ++index;
            }
            if (index == MaxGamepads)
            {
                return;
            }

            SDL_Gamepad* gamepad = SDL_OpenGamepad(id);
            if (gamepad == nullptr)
            {
                if (std::ranges::find(m_Failed, id) == m_Failed.end())
                {
                    const char* name = SDL_GetGamepadNameForID(id);
                    QUASAR_ENGINE_WARN("Cannot open the gamepad '{}': {}", name != nullptr ? name : "", SDL_GetError());
                    m_Failed.push_back(id);
                }
                continue;
            }
            m_Gamepads.at(index) = gamepad;
        }
    }
}
