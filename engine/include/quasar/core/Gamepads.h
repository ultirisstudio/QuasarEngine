#pragma once

/// \file
/// \brief The connected gamepads: their state, read once per frame, and the events sent when it changes.
/// \author Kurogenshi
/// \date 2026-11-06

#include <quasar/core/GamepadCodes.h>
#include <quasar/events/Event.h>

#include <array>
#include <functional>
#include <optional>
#include <string>

namespace Quasar
{
    /// @brief State of the buttons and axes of a gamepad.
    struct GamepadState
    {
        std::array<bool, GamepadButtonCount> Buttons{}; ///< True for each button held down, indexed by GamepadButton.
        std::array<float, GamepadAxisCount> Axes{};     ///< Value of each axis, indexed by GamepadAxis.

        /// @brief Returns true if the button is held down.
        bool IsPressed(GamepadButton button) const { return Buttons.at(static_cast<std::size_t>(button)); }

        /// @brief Returns the value of the axis.
        float GetAxis(GamepadAxis axis) const { return Axes.at(static_cast<std::size_t>(axis)); }
    };

    /// @brief What is read from a connected gamepad, before the dead zones are applied.
    struct GamepadReading
    {
        std::string Name;   ///< Name of the gamepad, for example "Xbox Controller".
        GamepadState State; ///< Raw state: the stick axes go from -1 to 1, and the triggers from 0, at rest, to 1.
    };

    /// @brief What is read from every gamepad id at once: a reading for each connected gamepad, no value for the others.
    using GamepadReadings = std::array<std::optional<GamepadReading>, MaxGamepads>;

    /// @brief Settings that filter the axes of the gamepads.
    struct GamepadSettings
    {
        /// Distance from the center under which a stick reads as 0. A stick at rest is never exactly at the center.
        /// The dead zone is a circle, so diagonals stay diagonals, and the values past it are scaled to start from 0.
        float StickDeadZone = 0.15f;

        /// Value under which a trigger reads as 0, on its range from 0 to 1. The values past it are scaled to start
        /// from 0.
        float TriggerDeadZone = 0.05f;

        /// Change needed to send a GamepadAxisMovedEvent, so that a moving stick does not send an event every frame
        /// for tiny changes. Reaching 0 or the end of the range always sends an event.
        float AxisEventThreshold = 0.01f;
    };

    /// @brief The gamepads connected to the computer, up to MaxGamepads.
    ///
    /// The application owns one and calls Update() once per frame, after the window processed the messages of the
    /// operating system. Update() reads the state of every gamepad with SDL, compares it with the previous frame, and
    /// sends an event for each change. The connection and disconnection of a gamepad are also found by Update().
    ///
    /// Only devices that SDL knows as gamepads are listed, so that the buttons and axes are at the same place on every
    /// gamepad. SDL has its own drivers for many gamepads, such as the Steam Controller and the PlayStation and Switch
    /// controllers, and the mappings of the SDL_GameControllerDB project for the others.
    class Gamepads
    {
    public:
        /// @brief Function that receives the events of the gamepads.
        using EventCallback = std::function<void(Event&)>;

        /// @brief Function that reads every gamepad id once per Update().
        using Reader = std::function<GamepadReadings()>;

        /// @brief Creates the gamepads, read with SDL. The gamepad support of SDL is started here and stopped when the
        /// gamepads are destroyed or when another reader is set. If it cannot start, the reason is logged as an engine
        /// error and no gamepad is ever connected.
        Gamepads();

        /// @brief Creates the gamepads, read with another function, for example by the tests.
        explicit Gamepads(Reader reader);

        /// @brief Reads every gamepad, keeps its new state, and sends the events of the changes since the last call.
        ///
        /// For each gamepad, in this order:
        /// - a gamepad that is connected sends GamepadConnectedEvent. The buttons already held down send no event.
        /// - a gamepad that is disconnected sends GamepadButtonReleasedEvent for each button still held down, then
        ///   GamepadDisconnectedEvent.
        /// - each button pressed or released sends GamepadButtonPressedEvent or GamepadButtonReleasedEvent.
        /// - each axis that moved enough sends GamepadAxisMovedEvent. See GamepadSettings::AxisEventThreshold.
        ///
        /// The state is updated before the events are sent, so the functions of this class give the new state.
        /// @param send The function that receives the events.
        void Update(const EventCallback& send);

        /// @brief Returns true if a gamepad is connected with this id.
        bool IsConnected(GamepadId gamepad) const;

        /// @brief Returns the name of the gamepad, or an empty string if no gamepad is connected with this id.
        const std::string& GetName(GamepadId gamepad) const;

        /// @brief Returns true if the button of the gamepad is held down. False when the gamepad is not connected.
        bool IsButtonPressed(GamepadId gamepad, GamepadButton button) const;

        /// @brief Returns the value of the axis of the gamepad, after the dead zones. 0 when the gamepad is not
        /// connected.
        ///
        /// Stick axes go from -1 to 1, and triggers from 0 to 1. The value is the one of the last Update(), which can
        /// differ slightly from the last GamepadAxisMovedEvent. See GamepadSettings::AxisEventThreshold.
        float GetAxis(GamepadId gamepad, GamepadAxis axis) const;

        /// @brief Returns the settings of the dead zones and of the events, which can be changed at any time.
        GamepadSettings& GetSettings() { return m_Settings; }

        /// @brief Returns the settings of the dead zones and of the events.
        const GamepadSettings& GetSettings() const { return m_Settings; }

        /// @brief Replaces the function that reads the gamepads, for example to simulate gamepads in the tests.
        void SetReader(Reader reader);

    private:
        struct Slot
        {
            bool Connected = false;
            std::string Name;
            GamepadState Current;
            GamepadState Reported;
        };

        const Slot* FindConnected(GamepadId gamepad) const;
        void UpdateSlot(GamepadId gamepad, const std::optional<GamepadReading>& reading, Slot& slot, const EventCallback& send);

        Reader m_Reader;
        GamepadSettings m_Settings;
        std::array<Slot, MaxGamepads> m_Slots{};
    };
}
