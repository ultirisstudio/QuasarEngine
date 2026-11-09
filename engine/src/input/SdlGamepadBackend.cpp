#include <input/SdlGamepadBackend.h>

#include <quasar/log/Log.h>

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

        // SDL gives the positions and the pressures from 0 to 1, but they are clamped again in case a driver goes past
        void ReadTouchpads(SDL_Gamepad* gamepad, GamepadReading& reading)
        {
            const int touchpads = std::clamp(SDL_GetNumGamepadTouchpads(gamepad), 0, static_cast<int>(MaxGamepadTouchpads));
            reading.TouchpadCount = static_cast<std::size_t>(touchpads);
            for (int touchpad = 0; touchpad < touchpads; ++touchpad)
            {
                const int fingers = std::clamp(SDL_GetNumGamepadTouchpadFingers(gamepad, touchpad), 0, static_cast<int>(MaxGamepadTouchpadFingers));
                for (int finger = 0; finger < fingers; ++finger)
                {
                    bool down = false;
                    float x = 0.0f;
                    float y = 0.0f;
                    float pressure = 0.0f;
                    if (!SDL_GetGamepadTouchpadFinger(gamepad, touchpad, finger, &down, &x, &y, &pressure) || !down)
                    {
                        continue;
                    }

                    GamepadFinger& state = reading.State.Touchpads.at(static_cast<std::size_t>(touchpad)).Fingers.at(static_cast<std::size_t>(finger));
                    state.Down = true;
                    state.Position = {std::clamp(x, 0.0f, 1.0f), std::clamp(y, 0.0f, 1.0f)};
                    state.Pressure = std::clamp(pressure, 0.0f, 1.0f);
                }
            }
        }

        constexpr std::array<SDL_GamepadButton, GamepadTouchSensorCount> SteamTouchSensorButtons{SDL_GAMEPAD_BUTTON_MISC3, SDL_GAMEPAD_BUTTON_MISC4,
                                                                                                 SDL_GAMEPAD_BUTTON_MISC5, SDL_GAMEPAD_BUTTON_MISC6};

        bool HasSteamTouchSensors(SDL_Gamepad* gamepad)
        {
            return SDL_GetGamepadType(gamepad) == SDL_GAMEPAD_TYPE_STEAM;
        }

        bool IsSteamTouchSensor(SDL_Gamepad* gamepad, SDL_GamepadButton button)
        {
            return HasSteamTouchSensors(gamepad) && std::ranges::find(SteamTouchSensorButtons, button) != SteamTouchSensorButtons.end();
        }

        void ReadTouchSensors(SDL_Gamepad* gamepad, GamepadReading& reading)
        {
            if (!HasSteamTouchSensors(gamepad))
            {
                return;
            }
            for (std::size_t sensor = 0; sensor < GamepadTouchSensorCount; ++sensor)
            {
                reading.State.TouchSensors.at(sensor) = SDL_GetGamepadButton(gamepad, SteamTouchSensorButtons.at(sensor));
            }
        }

        GamepadReading ReadGamepad(SDL_Gamepad* gamepad)
        {
            GamepadReading reading;
            const char* name = SDL_GetGamepadName(gamepad);
            reading.Name = name != nullptr ? name : "";
            reading.Type = static_cast<GamepadType>(SDL_GetGamepadType(gamepad));

            for (std::size_t index = 0; index < GamepadButtonCount; ++index)
            {
                const auto button = static_cast<SDL_GamepadButton>(index);
                reading.State.Buttons.at(index) = !IsSteamTouchSensor(gamepad, button) && SDL_GetGamepadButton(gamepad, button);
            }
            for (std::size_t axis = 0; axis < GamepadAxisCount; ++axis)
            {
                const float value = static_cast<float>(SDL_GetGamepadAxis(gamepad, static_cast<SDL_GamepadAxis>(axis))) / AxisRange;
                reading.State.Axes.at(axis) = std::clamp(value, -1.0f, 1.0f);
            }

            ReadTouchpads(gamepad, reading);
            ReadTouchSensors(gamepad, reading);
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
