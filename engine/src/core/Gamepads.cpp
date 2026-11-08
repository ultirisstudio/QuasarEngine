#include <quasar/core/Gamepads.h>
#include <quasar/events/GamepadEvent.h>

#include <core/SdlGamepadBackend.h>

#include <algorithm>
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
            if (!slot.Connected)
            {
                return;
            }

            const GamepadState held = slot.Reported;
            slot.Current = GamepadState{};
            slot.Reported = GamepadState{};
            for (std::size_t button = 0; button < GamepadButtonCount; ++button)
            {
                if (held.Buttons.at(button))
                {
                    GamepadButtonReleasedEvent event(gamepad, static_cast<GamepadButton>(button));
                    send(event);
                }
            }

            slot = Slot{};
            GamepadDisconnectedEvent event(gamepad);
            send(event);
            return;
        }

        const GamepadState current = Filter(reading->State, m_Settings);

        if (!slot.Connected)
        {
            slot = Slot{.Connected = true, .Name = reading->Name, .Type = reading->Type, .Current = current, .Reported = current};
            GamepadConnectedEvent event(gamepad, slot.Name);
            send(event);
            return;
        }

        const GamepadState previous = slot.Reported;
        slot.Current = current;
        slot.Reported.Buttons = current.Buttons;
        for (std::size_t axis = 0; axis < GamepadAxisCount; ++axis)
        {
            if (ShouldReportAxis(previous.Axes.at(axis), current.Axes.at(axis), m_Settings.AxisEventThreshold))
            {
                slot.Reported.Axes.at(axis) = current.Axes.at(axis);
            }
        }

        for (std::size_t button = 0; button < GamepadButtonCount; ++button)
        {
            const bool pressed = current.Buttons.at(button);
            if (pressed == previous.Buttons.at(button))
            {
                continue;
            }
            if (pressed)
            {
                GamepadButtonPressedEvent event(gamepad, static_cast<GamepadButton>(button));
                send(event);
            }
            else
            {
                GamepadButtonReleasedEvent event(gamepad, static_cast<GamepadButton>(button));
                send(event);
            }
        }
        for (std::size_t axis = 0; axis < GamepadAxisCount; ++axis)
        {
            if (slot.Reported.Axes.at(axis) != previous.Axes.at(axis))
            {
                GamepadAxisMovedEvent event(gamepad, static_cast<GamepadAxis>(axis), slot.Reported.Axes.at(axis));
                send(event);
            }
        }
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
        return slot != nullptr && slot->Current.IsPressed(button);
    }

    float Gamepads::GetAxis(GamepadId gamepad, GamepadAxis axis) const
    {
        const Slot* slot = FindConnected(gamepad);
        return slot != nullptr ? slot->Current.GetAxis(axis) : 0.0f;
    }
}
