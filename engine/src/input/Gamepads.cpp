#include <quasar/events/GamepadEvent.h>
#include <quasar/input/Gamepads.h>

#include <input/SdlGamepadBackend.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace Quasar
{
    namespace
    {
        constexpr auto LeftX = static_cast<std::size_t>(GamepadAxis::LeftX);
        constexpr auto LeftY = static_cast<std::size_t>(GamepadAxis::LeftY);
        constexpr auto RightX = static_cast<std::size_t>(GamepadAxis::RightX);
        constexpr auto RightY = static_cast<std::size_t>(GamepadAxis::RightY);
        constexpr auto LeftTrigger = static_cast<std::size_t>(GamepadAxis::LeftTrigger);
        constexpr auto RightTrigger = static_cast<std::size_t>(GamepadAxis::RightTrigger);

        void FilterStick(float& x, float& y, float deadZone)
        {
            const float length = std::hypot(x, y);
            if (length <= deadZone || length == 0.0f)
            {
                x = 0.0f;
                y = 0.0f;
                return;
            }

            const float scaled = std::min(1.0f, (length - deadZone) / (1.0f - deadZone));
            x = std::clamp(x / length * scaled, -1.0f, 1.0f);
            y = std::clamp(y / length * scaled, -1.0f, 1.0f);
        }

        float FilterTrigger(float raw, float deadZone)
        {
            const float value = std::clamp(raw, 0.0f, 1.0f);
            if (value <= deadZone)
            {
                return 0.0f;
            }
            return std::min(1.0f, (value - deadZone) / (1.0f - deadZone));
        }

        GamepadState Filter(const GamepadState& raw, const GamepadSettings& settings)
        {
            GamepadState state = raw;
            FilterStick(state.Axes[LeftX], state.Axes[LeftY], settings.StickDeadZone);
            FilterStick(state.Axes[RightX], state.Axes[RightY], settings.StickDeadZone);
            state.Axes[LeftTrigger] = FilterTrigger(raw.Axes[LeftTrigger], settings.TriggerDeadZone);
            state.Axes[RightTrigger] = FilterTrigger(raw.Axes[RightTrigger], settings.TriggerDeadZone);
            return state;
        }

        bool ShouldReportAxis(float reported, float current, float threshold)
        {
            if (current == reported)
            {
                return false;
            }
            const bool reachedLimit = current == 0.0f || std::abs(current) == 1.0f;
            return reachedLimit || std::abs(current - reported) >= threshold;
        }

        void SendButtonChanges(GamepadId gamepad, const GamepadState& previous, const GamepadState& current, const Gamepads::EventCallback& send)
        {
            for (std::size_t index = 0; index < GamepadButtonCount; ++index)
            {
                const auto button = static_cast<GamepadButton>(index);
                if (current.IsPressed(button) && !previous.IsPressed(button))
                {
                    GamepadButtonPressedEvent event(gamepad, button);
                    send(event);
                }
                else if (previous.IsPressed(button) && !current.IsPressed(button))
                {
                    GamepadButtonReleasedEvent event(gamepad, button);
                    send(event);
                }
            }
        }

        void SendAxisChanges(GamepadId gamepad, const GamepadState& current, std::array<float, GamepadAxisCount>& reported, float threshold,
                             const Gamepads::EventCallback& send)
        {
            for (std::size_t index = 0; index < GamepadAxisCount; ++index)
            {
                const auto axis = static_cast<GamepadAxis>(index);
                if (ShouldReportAxis(reported.at(index), current.GetAxis(axis), threshold))
                {
                    reported.at(index) = current.GetAxis(axis);
                    GamepadAxisMovedEvent event(gamepad, axis, reported.at(index));
                    send(event);
                }
            }
        }

        void SendFingerChanges(GamepadId gamepad, std::size_t touchpadCount, const GamepadState& previous, const GamepadState& current,
                               const Gamepads::EventCallback& send)
        {
            for (std::size_t touchpad = 0; touchpad < touchpadCount; ++touchpad)
            {
                for (std::size_t finger = 0; finger < MaxGamepadTouchpadFingers; ++finger)
                {
                    const GamepadFinger& before = previous.Touchpads.at(touchpad).Fingers.at(finger);
                    const GamepadFinger& after = current.Touchpads.at(touchpad).Fingers.at(finger);
                    if (after.Down && !before.Down)
                    {
                        GamepadFingerDownEvent event(gamepad, touchpad, finger, after.Position, after.Pressure);
                        send(event);
                    }
                    else if (before.Down && !after.Down)
                    {
                        GamepadFingerUpEvent event(gamepad, touchpad, finger, before.Position, 0.0f);
                        send(event);
                    }
                    else if (after.Down && after != before)
                    {
                        GamepadFingerMovedEvent event(gamepad, touchpad, finger, after.Position, after.Pressure);
                        send(event);
                    }
                }
            }
        }
    }

    Gamepads::Gamepads() : m_Backend(CreateScope<SdlGamepadBackend>()) {}

    Gamepads::Gamepads(Scope<GamepadBackend> backend) : m_Backend(std::move(backend)) {}

    void Gamepads::SetBackend(Scope<GamepadBackend> backend)
    {
        m_Backend = std::move(backend);
    }

    void Gamepads::Update(const EventCallback& send)
    {
        const GamepadReadings readings = m_Backend ? m_Backend->Read() : GamepadReadings{};
        for (std::size_t index = 0; index < MaxGamepads; ++index)
        {
            UpdateSlot(static_cast<GamepadId>(index), readings.at(index), m_Slots.at(index), send);
        }
    }

    void Gamepads::UpdateSlot(GamepadId gamepad, const std::optional<GamepadReading>& reading, Slot& slot, const EventCallback& send)
    {
        if (!reading)
        {
            if (slot.Connected)
            {
                Disconnect(gamepad, slot, send);
            }
            return;
        }

        const GamepadState current = Filter(reading->State, m_Settings);
        if (!slot.Connected)
        {
            Connect(gamepad, *reading, current, slot, send);
            return;
        }

        const GamepadState previous = slot.State;
        slot.State = current;
        SendButtonChanges(gamepad, previous, current, send);
        SendAxisChanges(gamepad, current, slot.ReportedAxes, m_Settings.AxisEventThreshold, send);
        SendFingerChanges(gamepad, slot.TouchpadCount, previous, current, send);
    }

    void Gamepads::Connect(GamepadId gamepad, const GamepadReading& reading, const GamepadState& state, Slot& slot, const EventCallback& send)
    {
        slot = Slot{.Connected = true,
                    .Name = reading.Name,
                    .Type = reading.Type,
                    .TouchpadCount = std::min(reading.TouchpadCount, MaxGamepadTouchpads),
                    .State = state,
                    .ReportedAxes = state.Axes};
        GamepadConnectedEvent event(gamepad, slot.Name);
        send(event);
    }

    void Gamepads::Disconnect(GamepadId gamepad, Slot& slot, const EventCallback& send)
    {
        const GamepadState held = slot.State;
        const std::size_t touchpadCount = slot.TouchpadCount;
        slot = Slot{};

        SendButtonChanges(gamepad, held, GamepadState{}, send);
        SendFingerChanges(gamepad, touchpadCount, held, GamepadState{}, send);
        GamepadDisconnectedEvent event(gamepad);
        send(event);
    }

    const Gamepads::Slot* Gamepads::FindConnected(GamepadId gamepad) const
    {
        const auto index = static_cast<std::size_t>(gamepad);
        if (index >= MaxGamepads || !m_Slots.at(index).Connected)
        {
            return nullptr;
        }
        return &m_Slots.at(index);
    }

    bool Gamepads::IsConnected(GamepadId gamepad) const
    {
        return FindConnected(gamepad) != nullptr;
    }

    const std::string& Gamepads::GetName(GamepadId gamepad) const
    {
        static const std::string empty;
        const Slot* slot = FindConnected(gamepad);
        return slot != nullptr ? slot->Name : empty;
    }

    GamepadType Gamepads::GetType(GamepadId gamepad) const
    {
        const Slot* slot = FindConnected(gamepad);
        return slot != nullptr ? slot->Type : GamepadType::Unknown;
    }

    bool Gamepads::Rumble(GamepadId gamepad, const GamepadRumble& rumble)
    {
        return m_Backend != nullptr && IsConnected(gamepad) && m_Backend->Rumble(gamepad, rumble);
    }

    bool Gamepads::IsButtonPressed(GamepadId gamepad, GamepadButton button) const
    {
        const Slot* slot = FindConnected(gamepad);
        return slot != nullptr && slot->State.IsPressed(button);
    }

    float Gamepads::GetAxis(GamepadId gamepad, GamepadAxis axis) const
    {
        const Slot* slot = FindConnected(gamepad);
        return slot != nullptr ? slot->State.GetAxis(axis) : 0.0f;
    }

    bool Gamepads::IsTouched(GamepadId gamepad, GamepadTouchSensor sensor) const
    {
        const Slot* slot = FindConnected(gamepad);
        return slot != nullptr && slot->State.IsTouched(sensor);
    }

    std::size_t Gamepads::GetTouchpadCount(GamepadId gamepad) const
    {
        const Slot* slot = FindConnected(gamepad);
        return slot != nullptr ? slot->TouchpadCount : 0;
    }

    GamepadFinger Gamepads::GetTouchpadFinger(GamepadId gamepad, std::size_t touchpad, std::size_t finger) const
    {
        const Slot* slot = FindConnected(gamepad);
        if (slot == nullptr || touchpad >= slot->TouchpadCount || finger >= MaxGamepadTouchpadFingers)
        {
            return {};
        }
        return slot->State.Touchpads.at(touchpad).Fingers.at(finger);
    }
}
