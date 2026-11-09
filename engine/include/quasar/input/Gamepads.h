#pragma once

/// \file
/// \brief The connected gamepads: their state, read once per frame, and the events sent when it changes.
/// \author Kurogenshi
/// \date 2026-11-06

#include <quasar/core/Base.h>
#include <quasar/events/Event.h>
#include <quasar/input/GamepadBackend.h>
#include <quasar/input/GamepadCodes.h>

#include <quasar/input/GamepadState.h>

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>

namespace Quasar
{
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
    /// Some gamepads have touchpads, like the PlayStation 4 and 5 controllers and the new Steam Controller. Their
    /// fingers are read like the buttons: GetTouchpadFinger() gives their state, and the finger events give their
    /// changes.
    ///
    /// Only devices that SDL knows as gamepads are listed, so that the buttons and axes are at the same place on every
    /// gamepad. SDL has its own drivers for many gamepads, such as the Steam Controller and the PlayStation and Switch
    /// controllers, and the mappings of the SDL_GameControllerDB project for the others.
    ///
    /// A game launched by Steam does not see the physical gamepads: Steam Input replaces each of them with a virtual
    /// Xbox controller, whose buttons and axes are the ones the player chose in Steam. The touchpads and the buttons from
    /// GamepadButton::Misc1 are then not available, and GetType() still gives the kind of the physical gamepad. A Steam
    /// Controller always goes through Steam Input, even when it is disabled for the game. While Steam runs, it also
    /// turns a Steam Controller into a keyboard and a mouse for the programs it did not launch, so a game started
    /// outside Steam receives key and mouse events from it too.
    class Gamepads
    {
    public:
        /// @brief Function that receives the events of the gamepads.
        using EventCallback = std::function<void(Event&)>;

        /// @brief Creates the gamepads, read with SDL. The gamepad support of SDL is started here and stopped when the
        /// gamepads are destroyed or when another backend is set. If it cannot start, the reason is logged as an engine
        /// error and no gamepad is ever connected.
        Gamepads();

        /// @brief Creates the gamepads, read from another backend, for example by the tests.
        /// @param backend The backend, or nullptr for no gamepad at all.
        explicit Gamepads(Scope<GamepadBackend> backend);

        /// @brief Reads every gamepad, keeps its new state, and sends the events of the changes since the last call.
        ///
        /// For each gamepad, in this order:
        /// - a gamepad that is connected sends GamepadConnectedEvent. The buttons already held down and the fingers
        ///   already down send no event.
        /// - a gamepad that is disconnected sends GamepadButtonReleasedEvent for each button still held down,
        ///   GamepadFingerUpEvent for each finger still down, then GamepadDisconnectedEvent.
        /// - each button pressed or released sends GamepadButtonPressedEvent or GamepadButtonReleasedEvent.
        /// - each axis that moved enough sends GamepadAxisMovedEvent. See GamepadSettings::AxisEventThreshold.
        /// - each finger that touches a touchpad, moves on it or leaves it sends GamepadFingerDownEvent,
        ///   GamepadFingerMovedEvent or GamepadFingerUpEvent.
        ///
        /// The state is updated before the events are sent, so the functions of this class give the new state.
        /// @param send The function that receives the events.
        void Update(const EventCallback& send);

        /// @brief Returns true if a gamepad is connected with this id.
        bool IsConnected(GamepadId gamepad) const;

        /// @brief Returns the name of the gamepad, or an empty string if no gamepad is connected with this id.
        const std::string& GetName(GamepadId gamepad) const;

        /// @brief Returns the kind of gamepad, or GamepadType::Unknown if no gamepad is connected with this id.
        GamepadType GetType(GamepadId gamepad) const;

        /// @brief Returns true if the button of the gamepad is held down. False when the gamepad is not connected.
        bool IsButtonPressed(GamepadId gamepad, GamepadButton button) const;

        /// @brief Returns the value of the axis of the gamepad, after the dead zones. 0 when the gamepad is not
        /// connected.
        ///
        /// Stick axes go from -1 to 1, and triggers from 0 to 1. The value is the one of the last Update(), which can
        /// differ slightly from the last GamepadAxisMovedEvent. See GamepadSettings::AxisEventThreshold.
        float GetAxis(GamepadId gamepad, GamepadAxis axis) const;

        /// @brief Returns true if the touch sensor of the gamepad is touched. False when the gamepad is not connected or
        /// has no such sensor.
        ///
        /// The touch sensors send no event: they change each time the gamepad is held, put down or moved in the hands.
        bool IsTouched(GamepadId gamepad, GamepadTouchSensor sensor) const;

        /// @brief Returns the number of touchpads of the gamepad, or 0 if no gamepad is connected with this id.
        std::size_t GetTouchpadCount(GamepadId gamepad) const;

        /// @brief Returns a finger on a touchpad of the gamepad.
        /// @param gamepad The id of the gamepad.
        /// @param touchpad The index of the touchpad, from 0 to GetTouchpadCount() - 1.
        /// @param finger The index of the finger, from 0 to MaxGamepadTouchpadFingers - 1.
        /// @return The finger. It is not down if the gamepad is not connected, or if the touchpad or the finger does
        /// not exist.
        GamepadFinger GetTouchpadFinger(GamepadId gamepad, std::size_t touchpad, std::size_t finger) const;

        /// @brief Returns the settings of the dead zones and of the events, which can be changed at any time.
        GamepadSettings& GetSettings() { return m_Settings; }

        /// @brief Returns the settings of the dead zones and of the events.
        const GamepadSettings& GetSettings() const { return m_Settings; }

        /// @brief Starts or stops the rumble of a gamepad. See GamepadRumble.
        /// @return False if no gamepad is connected with this id, or if it cannot rumble.
        bool Rumble(GamepadId gamepad, const GamepadRumble& rumble);

        /// @brief Replaces the backend of the gamepads, for example to simulate gamepads in the tests.
        /// @param backend The new backend, or nullptr for no gamepad at all.
        void SetBackend(Scope<GamepadBackend> backend);

    private:
        struct Slot
        {
            bool Connected = false;
            std::string Name;
            GamepadType Type = GamepadType::Unknown;
            std::size_t TouchpadCount = 0;
            GamepadState State;
            std::array<float, GamepadAxisCount> ReportedAxes{};
        };

        const Slot* FindConnected(GamepadId gamepad) const;
        void UpdateSlot(GamepadId gamepad, const std::optional<GamepadReading>& reading, Slot& slot, const EventCallback& send);
        static void Connect(GamepadId gamepad, const GamepadReading& reading, const GamepadState& state, Slot& slot, const EventCallback& send);
        static void Disconnect(GamepadId gamepad, Slot& slot, const EventCallback& send);

        Scope<GamepadBackend> m_Backend;
        GamepadSettings m_Settings;
        std::array<Slot, MaxGamepads> m_Slots{};
    };
}
