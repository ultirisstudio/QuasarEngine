#include <quasar/core/Gamepads.h>
#include <quasar/events/GamepadEvent.h>

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
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

        std::optional<GamepadReading> ReadGlfwGamepad(GamepadId gamepad)
        {
            const int id = static_cast<int>(gamepad);
            GLFWgamepadstate glfwState{};
            if (glfwGetGamepadState(id, &glfwState) == GLFW_FALSE)
            {
                return std::nullopt;
            }

            GamepadReading reading;
            const char* name = glfwGetGamepadName(id);
            reading.Name = name != nullptr ? name : "";
            std::ranges::transform(glfwState.buttons, reading.State.Buttons.begin(), [](unsigned char button) { return button == GLFW_PRESS; });
            std::ranges::copy(glfwState.axes, reading.State.Axes.begin());
            return reading;
        }

        // Applies a circular dead zone to the two axes of a stick, then scales the values past it to start from 0
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

        // Moves a trigger from -1..1 to 0..1, then applies its dead zone
        float FilterTrigger(float raw, float deadZone)
        {
            const float value = std::clamp((raw + 1.0f) * 0.5f, 0.0f, 1.0f);
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

        // A change is sent when it is large enough, or when the axis reaches its rest position or the end of its range
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

    Gamepads::Gamepads() : m_Reader(ReadGlfwGamepad) {}

    Gamepads::Gamepads(Reader reader) : m_Reader(std::move(reader)) {}

    void Gamepads::SetReader(Reader reader)
    {
        m_Reader = std::move(reader);
    }

    void Gamepads::Update(const EventCallback& send)
    {
        for (std::size_t index = 0; index < MaxGamepads; ++index)
        {
            UpdateSlot(static_cast<GamepadId>(index), m_Slots.at(index), send);
        }
    }

    void Gamepads::UpdateSlot(GamepadId gamepad, Slot& slot, const EventCallback& send)
    {
        const std::optional<GamepadReading> reading = m_Reader ? m_Reader(gamepad) : std::nullopt;

        if (!reading)
        {
            if (!slot.Connected)
            {
                return;
            }

            // The buttons held down are released first, so that nothing stays pressed after the gamepad is gone
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
            slot = Slot{.Connected = true, .Name = reading->Name, .Current = current, .Reported = current};
            GamepadConnectedEvent event(gamepad, slot.Name);
            send(event);
            return;
        }

        // The new state is kept before the events are sent, so that the receivers read it
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