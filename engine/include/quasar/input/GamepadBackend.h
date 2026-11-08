#pragma once

/// \file
/// \brief Interface of the code that reads the gamepads and makes them rumble, with SDL in the engine.
/// \author Kurogenshi
/// \date 2026-11-08

#include <quasar/input/GamepadCodes.h>
#include <quasar/input/GamepadState.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>

namespace Quasar
{
    /// @brief What is read from a connected gamepad, before the dead zones are applied.
    struct GamepadReading
    {
        std::string Name;                        ///< Name of the gamepad, for example "Xbox Controller".
        GamepadType Type = GamepadType::Unknown; ///< Kind of gamepad.
        std::size_t TouchpadCount = 0;           ///< Number of touchpads of the gamepad, up to MaxGamepadTouchpads.
        GamepadState State;                      ///< Raw state: the stick axes go from -1 to 1, and the triggers from 0, at rest, to 1.
    };

    /// @brief What is read from every gamepad id at once: a reading for each connected gamepad, no value for the others.
    using GamepadReadings = std::array<std::optional<GamepadReading>, MaxGamepads>;

    /// @brief A vibration of the two motors of a gamepad.
    ///
    /// A new rumble replaces the one that is running. A rumble with a duration of 0, or with both strengths at 0, stops
    /// the motors.
    struct GamepadRumble
    {
        float LowFrequency = 0.0f;             ///< Strength of the low frequency motor, usually the left one, from 0 to 1.
        float HighFrequency = 0.0f;            ///< Strength of the high frequency motor, usually the right one, from 0 to 1.
        std::chrono::milliseconds Duration{0}; ///< How long the motors run. SDL stops them after 65 seconds at most.
    };

    /// @brief Where the gamepads are read from, and where their rumble is sent.
    ///
    /// The engine reads the gamepads with SDL. The tests use their own backend to simulate gamepads.
    class GamepadBackend
    {
    public:
        GamepadBackend() = default;
        virtual ~GamepadBackend() = default;

        GamepadBackend(const GamepadBackend&) = delete;
        GamepadBackend& operator=(const GamepadBackend&) = delete;
        GamepadBackend(GamepadBackend&&) = delete;
        GamepadBackend& operator=(GamepadBackend&&) = delete;

        /// @brief Reads every gamepad id. Called once per Gamepads::Update().
        /// @return A reading for each connected gamepad, no value for the others.
        virtual GamepadReadings Read() = 0;

        /// @brief Starts or stops the rumble of a connected gamepad.
        /// @return False if the gamepad cannot rumble.
        virtual bool Rumble(GamepadId gamepad, const GamepadRumble& rumble) = 0;
    };
}
