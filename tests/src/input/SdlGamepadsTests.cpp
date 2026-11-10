#include "helpers/GamepadEvents.h"
#include "helpers/LogHelpers.h"

#include <quasar/core/Base.h>
#include <quasar/input/GamepadCodes.h>
#include <quasar/input/GamepadState.h>
#include <quasar/input/Gamepads.h>
#include <quasar/log/MemoryLogSink.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_stdinc.h>
#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <thread>
#include <vector>

using Quasar::GamepadAxis;
using Quasar::GamepadButton;
using Quasar::GamepadFinger;
using Quasar::GamepadId;
using Quasar::GamepadTouchSensor;
using Quasar::GamepadType;

using TestHelpers::Connected;
using TestHelpers::Disconnected;
using TestHelpers::FingerDown;
using TestHelpers::FingerUp;
using TestHelpers::Pressed;
using TestHelpers::RecordedEvent;
using TestHelpers::Released;
using TestHelpers::UpdateGamepads;

namespace
{
    constexpr const char* VirtualGamepadName = "Quasar virtual gamepad";
    constexpr std::size_t VirtualTouchpadCount = 2;

    struct MotorStrengths
    {
        Uint16 Low = 0;
        Uint16 High = 0;

        bool operator==(const MotorStrengths&) const = default;
    };

    class VirtualGamepad
    {
    public:
        explicit VirtualGamepad(Uint16 vendor = 0, Uint16 product = 0)
        {
            if (!SDL_InitSubSystem(SDL_INIT_JOYSTICK))
            {
                return;
            }
            m_Started = true;

            for (SDL_VirtualJoystickTouchpadDesc& touchpad : m_Touchpads)
            {
                touchpad.nfingers = static_cast<Uint16>(Quasar::MaxGamepadTouchpadFingers);
            }

            SDL_VirtualJoystickDesc description{};
            SDL_INIT_INTERFACE(&description);
            description.type = SDL_JOYSTICK_TYPE_GAMEPAD;
            description.naxes = static_cast<Uint16>(Quasar::GamepadAxisCount);
            description.nbuttons = static_cast<Uint16>(Quasar::GamepadButtonCount);
            description.ntouchpads = static_cast<Uint16>(m_Touchpads.size());
            description.touchpads = m_Touchpads.data();
            description.name = VirtualGamepadName;
            description.vendor_id = vendor;
            description.product_id = product;
            description.userdata = this;
            description.Rumble = [](void* userdata, Uint16 low, Uint16 high)
            {
                static_cast<VirtualGamepad*>(userdata)->m_Rumbles.push_back({.Low = low, .High = high});
                return true;
            };

            m_Id = SDL_AttachVirtualJoystick(&description);
            m_Joystick = SDL_OpenJoystick(m_Id);
        }

        ~VirtualGamepad()
        {
            Unplug();
            if (m_Started)
            {
                SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
            }
        }

        VirtualGamepad(const VirtualGamepad&) = delete;
        VirtualGamepad& operator=(const VirtualGamepad&) = delete;
        VirtualGamepad(VirtualGamepad&&) = delete;
        VirtualGamepad& operator=(VirtualGamepad&&) = delete;

        bool IsPlugged() const { return m_Id != 0 && m_Joystick != nullptr; }

        void SetButton(GamepadButton button, bool pressed) { SDL_SetJoystickVirtualButton(m_Joystick, static_cast<int>(button), pressed); }

        void SetAxis(GamepadAxis axis, Sint16 value) { SDL_SetJoystickVirtualAxis(m_Joystick, static_cast<int>(axis), value); }

        void SetFinger(int touchpad, int finger, const GamepadFinger& state)
        {
            SDL_SetJoystickVirtualTouchpad(m_Joystick, touchpad, finger, state.Down, state.Position.x, state.Position.y, state.Pressure);
        }

        const std::vector<MotorStrengths>& GetRumbles() const { return m_Rumbles; }

        void Unplug()
        {
            if (m_Joystick != nullptr)
            {
                SDL_CloseJoystick(m_Joystick);
                m_Joystick = nullptr;
            }
            if (m_Id != 0)
            {
                SDL_DetachVirtualJoystick(m_Id);
                m_Id = 0;
            }
        }

    private:
        bool m_Started = false;
        std::array<SDL_VirtualJoystickTouchpadDesc, VirtualTouchpadCount> m_Touchpads{};
        SDL_JoystickID m_Id = 0;
        SDL_Joystick* m_Joystick = nullptr;
        std::vector<MotorStrengths> m_Rumbles;
    };

    std::optional<GamepadId> FindVirtualGamepad(const Quasar::Gamepads& gamepads)
    {
        for (std::size_t index = 0; index < Quasar::MaxGamepads; ++index)
        {
            const auto gamepad = static_cast<GamepadId>(index);
            if (gamepads.IsConnected(gamepad) && gamepads.GetName(gamepad) == VirtualGamepadName)
            {
                return gamepad;
            }
        }
        return std::nullopt;
    }
}

TEST_CASE("A gamepad read with SDL is connected with its name")
{
    const TestHelpers::ScopedLog log;
    Quasar::Gamepads gamepads;
    const VirtualGamepad virtualGamepad;
    REQUIRE(virtualGamepad.IsPlugged());

    const std::vector<RecordedEvent> events = UpdateGamepads(gamepads);

    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());
    CHECK(std::ranges::count(events, Connected(*gamepad)) == 1);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("The buttons and axes of a gamepad read with SDL are at their place and in their range")
{
    Quasar::Gamepads gamepads;
    gamepads.GetSettings() = {.StickDeadZone = 0.0f, .TriggerDeadZone = 0.0f, .AxisEventThreshold = 0.0f};
    VirtualGamepad virtualGamepad;
    REQUIRE(virtualGamepad.IsPlugged());
    UpdateGamepads(gamepads);
    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());
    CHECK(gamepads.GetAxis(*gamepad, GamepadAxis::LeftTrigger) == 0.0f);

    virtualGamepad.SetButton(GamepadButton::LeftBumper, true);
    virtualGamepad.SetButton(GamepadButton::DPadLeft, true);
    virtualGamepad.SetAxis(GamepadAxis::LeftY, SDL_JOYSTICK_AXIS_MIN);
    virtualGamepad.SetAxis(GamepadAxis::RightX, SDL_JOYSTICK_AXIS_MAX);
    virtualGamepad.SetAxis(GamepadAxis::RightTrigger, SDL_JOYSTICK_AXIS_MAX);
    UpdateGamepads(gamepads);

    CHECK(gamepads.IsButtonPressed(*gamepad, GamepadButton::LeftBumper));
    CHECK(gamepads.IsButtonPressed(*gamepad, GamepadButton::DPadLeft));
    CHECK_FALSE(gamepads.IsButtonPressed(*gamepad, GamepadButton::A));
    CHECK(gamepads.GetAxis(*gamepad, GamepadAxis::LeftY) == -1.0f);
    CHECK(gamepads.GetAxis(*gamepad, GamepadAxis::RightX) == 1.0f);
    CHECK(gamepads.GetAxis(*gamepad, GamepadAxis::RightTrigger) == 1.0f);
    CHECK(gamepads.GetAxis(*gamepad, GamepadAxis::LeftTrigger) == 0.0f);
}

TEST_CASE("The extra buttons of a gamepad read with SDL are at their place")
{
    Quasar::Gamepads gamepads;
    VirtualGamepad virtualGamepad;
    REQUIRE(virtualGamepad.IsPlugged());
    UpdateGamepads(gamepads);
    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());

    virtualGamepad.SetButton(GamepadButton::RightPaddle1, true);
    virtualGamepad.SetButton(GamepadButton::Touchpad, true);
    virtualGamepad.SetButton(GamepadButton::Misc6, true);
    const std::vector<RecordedEvent> events = UpdateGamepads(gamepads);

    const std::vector<RecordedEvent> expected{Pressed(*gamepad, GamepadButton::RightPaddle1), Pressed(*gamepad, GamepadButton::Touchpad),
                                              Pressed(*gamepad, GamepadButton::Misc6)};
    CHECK(events == expected);
    CHECK_FALSE(gamepads.IsButtonPressed(*gamepad, GamepadButton::Misc1));
    CHECK_FALSE(gamepads.IsButtonPressed(*gamepad, GamepadButton::LeftPaddle1));
}

TEST_CASE("The touch sensors of a Steam controller read with SDL are touch sensors and not buttons")
{
    constexpr Uint16 valve = 0x28DE;
    constexpr Uint16 steamController = 0x1302;
    Quasar::Gamepads gamepads;
    VirtualGamepad virtualGamepad(valve, steamController);
    REQUIRE(virtualGamepad.IsPlugged());
    UpdateGamepads(gamepads);
    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());
    REQUIRE(gamepads.GetType(*gamepad) == GamepadType::SteamController);

    virtualGamepad.SetButton(GamepadButton::Misc1, true);
    virtualGamepad.SetButton(GamepadButton::Misc3, true);
    virtualGamepad.SetButton(GamepadButton::Misc6, true);
    const std::vector<RecordedEvent> events = UpdateGamepads(gamepads);

    CHECK(events == std::vector<RecordedEvent>{Pressed(*gamepad, GamepadButton::Misc1)});
    CHECK_FALSE(gamepads.IsButtonPressed(*gamepad, GamepadButton::Misc3));
    CHECK_FALSE(gamepads.IsButtonPressed(*gamepad, GamepadButton::Misc6));
    CHECK(gamepads.IsTouched(*gamepad, GamepadTouchSensor::LeftStick));
    CHECK(gamepads.IsTouched(*gamepad, GamepadTouchSensor::RightGrip));
    CHECK_FALSE(gamepads.IsTouched(*gamepad, GamepadTouchSensor::RightStick));
    CHECK_FALSE(gamepads.IsTouched(*gamepad, GamepadTouchSensor::LeftGrip));

    virtualGamepad.SetButton(GamepadButton::Misc3, false);
    UpdateGamepads(gamepads);

    CHECK_FALSE(gamepads.IsTouched(*gamepad, GamepadTouchSensor::LeftStick));
}

TEST_CASE("A gamepad that is not a Steam controller has no touch sensor")
{
    Quasar::Gamepads gamepads;
    VirtualGamepad virtualGamepad;
    REQUIRE(virtualGamepad.IsPlugged());
    UpdateGamepads(gamepads);
    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());

    virtualGamepad.SetButton(GamepadButton::Misc3, true);
    UpdateGamepads(gamepads);

    CHECK(gamepads.IsButtonPressed(*gamepad, GamepadButton::Misc3));
    CHECK_FALSE(gamepads.IsTouched(*gamepad, GamepadTouchSensor::LeftStick));
}

TEST_CASE("The fingers on the touchpads of a gamepad read with SDL are at their place")
{
    Quasar::Gamepads gamepads;
    VirtualGamepad virtualGamepad;
    REQUIRE(virtualGamepad.IsPlugged());
    UpdateGamepads(gamepads);
    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());
    CHECK(gamepads.GetTouchpadCount(*gamepad) == VirtualTouchpadCount);

    const GamepadFinger touching{.Down = true, .Position = {0.25f, 0.75f}, .Pressure = 0.5f};
    virtualGamepad.SetFinger(1, 1, touching);
    CHECK(UpdateGamepads(gamepads) == std::vector<RecordedEvent>{FingerDown(*gamepad, 1, 1, {0.25f, 0.75f}, 0.5f)});
    CHECK(gamepads.GetTouchpadFinger(*gamepad, 1, 1) == touching);
    CHECK_FALSE(gamepads.GetTouchpadFinger(*gamepad, 0, 0).Down);

    virtualGamepad.SetFinger(1, 1, {});
    CHECK(UpdateGamepads(gamepads) == std::vector<RecordedEvent>{FingerUp(*gamepad, 1, 1, {0.25f, 0.75f})});
}

TEST_CASE("A gamepad read with SDL releases its buttons and is disconnected when it is unplugged")
{
    Quasar::Gamepads gamepads;
    VirtualGamepad virtualGamepad;
    REQUIRE(virtualGamepad.IsPlugged());
    virtualGamepad.SetButton(GamepadButton::Y, true);
    UpdateGamepads(gamepads);
    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());

    virtualGamepad.Unplug();
    const std::vector<RecordedEvent> events = UpdateGamepads(gamepads);

    CHECK_FALSE(gamepads.IsConnected(*gamepad));
    CHECK(events == std::vector<RecordedEvent>{Released(*gamepad, GamepadButton::Y), Disconnected(*gamepad)});
}

TEST_CASE("A gamepad read with SDL never takes the id of a gamepad unplugged in the same frame")
{
    Quasar::Gamepads gamepads;
    VirtualGamepad first;
    REQUIRE(first.IsPlugged());
    UpdateGamepads(gamepads);
    const std::optional<GamepadId> firstId = FindVirtualGamepad(gamepads);
    REQUIRE(firstId.has_value());

    first.Unplug();
    const VirtualGamepad second;
    REQUIRE(second.IsPlugged());
    const std::vector<RecordedEvent> events = UpdateGamepads(gamepads);

    const std::optional<GamepadId> secondId = FindVirtualGamepad(gamepads);
    REQUIRE(secondId.has_value());
    CHECK(*secondId != *firstId);
    CHECK(std::ranges::count(events, Disconnected(*firstId)) == 1);
    CHECK(std::ranges::count(events, Connected(*secondId)) == 1);
}

TEST_CASE("A gamepad read with SDL waits for a free id when all the ids are taken")
{
    Quasar::Gamepads gamepads;
    std::vector<Quasar::Scope<VirtualGamepad>> virtualGamepads;
    for (std::size_t index = 0; index <= Quasar::MaxGamepads; ++index)
    {
        virtualGamepads.push_back(Quasar::CreateScope<VirtualGamepad>());
        REQUIRE(virtualGamepads.back()->IsPlugged());
    }
    UpdateGamepads(gamepads);
    const std::optional<GamepadId> firstId = FindVirtualGamepad(gamepads);
    REQUIRE(firstId.has_value());

    virtualGamepads.front()->Unplug();
    const std::vector<RecordedEvent> unplugEvents = UpdateGamepads(gamepads);
    const std::vector<RecordedEvent> nextEvents = UpdateGamepads(gamepads);

    CHECK(std::ranges::count(unplugEvents, Disconnected(*firstId)) == 1);
    CHECK(std::ranges::count(nextEvents, Connected(*firstId)) == 1);
    for (std::size_t index = 0; index < Quasar::MaxGamepads; ++index)
    {
        CHECK(gamepads.IsConnected(static_cast<GamepadId>(index)));
    }
}

TEST_CASE("The type of a gamepad read with SDL comes from its maker and its model")
{
    constexpr Uint16 sony = 0x054C;
    constexpr Uint16 dualSense = 0x0CE6;
    Quasar::Gamepads gamepads;
    const VirtualGamepad virtualGamepad(sony, dualSense);
    REQUIRE(virtualGamepad.IsPlugged());

    UpdateGamepads(gamepads);

    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());
    CHECK(gamepads.GetType(*gamepad) == GamepadType::PlayStation5);
}

TEST_CASE("A rumble of a gamepad read with SDL reaches its motors with the right strength")
{
    Quasar::Gamepads gamepads;
    const VirtualGamepad virtualGamepad;
    REQUIRE(virtualGamepad.IsPlugged());
    UpdateGamepads(gamepads);
    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());

    CHECK(gamepads.Rumble(*gamepad, {.LowFrequency = 1.0f, .HighFrequency = 0.5f, .Duration = std::chrono::milliseconds(1000)}));
    CHECK(gamepads.Rumble(*gamepad, {.LowFrequency = -1.0f, .HighFrequency = 2.0f, .Duration = std::chrono::milliseconds(1000)}));
    CHECK(gamepads.Rumble(*gamepad, {.LowFrequency = 1.0f, .HighFrequency = 1.0f, .Duration = std::chrono::milliseconds(0)}));

    const std::vector<MotorStrengths> expected{{.Low = 65535, .High = 32768}, {.Low = 0, .High = 65535}, {.Low = 0, .High = 0}};
    CHECK(virtualGamepad.GetRumbles() == expected);
}

TEST_CASE("A rumble of a gamepad read with SDL stops after its duration")
{
    Quasar::Gamepads gamepads;
    const VirtualGamepad virtualGamepad;
    REQUIRE(virtualGamepad.IsPlugged());
    UpdateGamepads(gamepads);
    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());

    REQUIRE(gamepads.Rumble(*gamepad, {.LowFrequency = 1.0f, .HighFrequency = 1.0f, .Duration = std::chrono::milliseconds(1)}));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    UpdateGamepads(gamepads);

    REQUIRE_FALSE(virtualGamepad.GetRumbles().empty());
    CHECK(virtualGamepad.GetRumbles().back() == MotorStrengths{});
}
