#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Application.h>
#include <quasar/core/Layer.h>
#include <quasar/events/EventDispatcher.h>
#include <quasar/events/GamepadEvent.h>
#include <quasar/input/GamepadBackend.h>
#include <quasar/input/GamepadCodes.h>
#include <quasar/input/GamepadState.h>
#include <quasar/input/Gamepads.h>
#include <quasar/input/Input.h>
#include <quasar/log/MemoryLogSink.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_joystick.h>
#include <SDL3/SDL_stdinc.h>
#include <doctest/doctest.h>
#include <glm/vec2.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using Quasar::EventType;
using Quasar::GamepadAxis;
using Quasar::GamepadButton;
using Quasar::GamepadFinger;
using Quasar::GamepadId;
using Quasar::GamepadTouchSensor;
using Quasar::GamepadType;

namespace
{
    constexpr GamepadId First{0};
    constexpr GamepadId Second{1};

    constexpr Quasar::GamepadSettings TestSettings{.StickDeadZone = 0.15f, .TriggerDeadZone = 0.05f, .AxisEventThreshold = 0.01f};

    struct RecordedEvent
    {
        EventType Type{};
        GamepadId Gamepad{};
        GamepadButton Button{};
        GamepadAxis Axis{};
        float Value = 0.0f;
        std::size_t Touchpad = 0;
        std::size_t Finger = 0;
        glm::vec2 Position{0.0f, 0.0f};
        float Pressure = 0.0f;

        bool operator==(const RecordedEvent&) const = default;
    };

    RecordedEvent Connected(GamepadId gamepad)
    {
        return {.Type = EventType::GamepadConnected, .Gamepad = gamepad};
    }

    RecordedEvent Disconnected(GamepadId gamepad)
    {
        return {.Type = EventType::GamepadDisconnected, .Gamepad = gamepad};
    }

    RecordedEvent Pressed(GamepadId gamepad, GamepadButton button)
    {
        return {.Type = EventType::GamepadButtonPressed, .Gamepad = gamepad, .Button = button};
    }

    RecordedEvent Released(GamepadId gamepad, GamepadButton button)
    {
        return {.Type = EventType::GamepadButtonReleased, .Gamepad = gamepad, .Button = button};
    }

    RecordedEvent FingerDown(GamepadId gamepad, std::size_t touchpad, std::size_t finger, glm::vec2 position, float pressure)
    {
        return {.Type = EventType::GamepadFingerDown, .Gamepad = gamepad, .Touchpad = touchpad, .Finger = finger, .Position = position, .Pressure = pressure};
    }

    RecordedEvent FingerMoved(GamepadId gamepad, std::size_t touchpad, std::size_t finger, glm::vec2 position, float pressure)
    {
        return {.Type = EventType::GamepadFingerMoved, .Gamepad = gamepad, .Touchpad = touchpad, .Finger = finger, .Position = position, .Pressure = pressure};
    }

    RecordedEvent FingerUp(GamepadId gamepad, std::size_t touchpad, std::size_t finger, glm::vec2 position)
    {
        return {.Type = EventType::GamepadFingerUp, .Gamepad = gamepad, .Touchpad = touchpad, .Finger = finger, .Position = position};
    }

    template <typename FingerEvent>
    void RecordFinger(Quasar::EventDispatcher& dispatcher, std::vector<RecordedEvent>& events)
    {
        dispatcher.Dispatch<FingerEvent>(
            [&events](const FingerEvent& finger)
            {
                events.push_back({.Type = FingerEvent::GetStaticType(),
                                  .Gamepad = finger.GetGamepad(),
                                  .Touchpad = finger.GetTouchpad(),
                                  .Finger = finger.GetFinger(),
                                  .Position = finger.GetPosition(),
                                  .Pressure = finger.GetPressure()});
                return false;
            });
    }

    void Record(Quasar::Event& event, std::vector<RecordedEvent>& events)
    {
        Quasar::EventDispatcher dispatcher(event);
        dispatcher.Dispatch<Quasar::GamepadConnectedEvent>(
            [&events](const Quasar::GamepadConnectedEvent& connected)
            {
                events.push_back(Connected(connected.GetGamepad()));
                return false;
            });
        dispatcher.Dispatch<Quasar::GamepadDisconnectedEvent>(
            [&events](const Quasar::GamepadDisconnectedEvent& disconnected)
            {
                events.push_back(Disconnected(disconnected.GetGamepad()));
                return false;
            });
        dispatcher.Dispatch<Quasar::GamepadButtonPressedEvent>(
            [&events](const Quasar::GamepadButtonPressedEvent& pressed)
            {
                events.push_back(Pressed(pressed.GetGamepad(), pressed.GetButton()));
                return false;
            });
        dispatcher.Dispatch<Quasar::GamepadButtonReleasedEvent>(
            [&events](const Quasar::GamepadButtonReleasedEvent& released)
            {
                events.push_back(Released(released.GetGamepad(), released.GetButton()));
                return false;
            });
        dispatcher.Dispatch<Quasar::GamepadAxisMovedEvent>(
            [&events](const Quasar::GamepadAxisMovedEvent& moved)
            {
                events.push_back({.Type = EventType::GamepadAxisMoved, .Gamepad = moved.GetGamepad(), .Axis = moved.GetAxis(), .Value = moved.GetValue()});
                return false;
            });
        RecordFinger<Quasar::GamepadFingerDownEvent>(dispatcher, events);
        RecordFinger<Quasar::GamepadFingerMovedEvent>(dispatcher, events);
        RecordFinger<Quasar::GamepadFingerUpEvent>(dispatcher, events);
    }

    std::vector<RecordedEvent> Update(Quasar::Gamepads& gamepads)
    {
        std::vector<RecordedEvent> events;
        gamepads.Update([&events](Quasar::Event& event) { Record(event, events); });
        return events;
    }

    struct RecordedRumble
    {
        GamepadId Gamepad{};
        Quasar::GamepadRumble Rumble;
    };

    class FakeGamepads
    {
    public:
        Quasar::Scope<Quasar::GamepadBackend> CreateBackend() { return Quasar::CreateScope<Backend>(*this); }

        void Connect(GamepadId gamepad, std::string name = "Test gamepad", GamepadType type = GamepadType::Standard)
        {
            Quasar::GamepadReading reading;
            reading.Name = std::move(name);
            reading.Type = type;
            m_Readings[gamepad] = reading;
        }

        void Disconnect(GamepadId gamepad) { m_Readings.erase(gamepad); }

        void SetButton(GamepadId gamepad, GamepadButton button, bool pressed)
        {
            m_Readings.at(gamepad).State.Buttons.at(static_cast<std::size_t>(button)) = pressed;
        }

        void SetAxis(GamepadId gamepad, GamepadAxis axis, float raw) { m_Readings.at(gamepad).State.Axes.at(static_cast<std::size_t>(axis)) = raw; }

        void SetTouchpadCount(GamepadId gamepad, std::size_t count) { m_Readings.at(gamepad).TouchpadCount = count; }

        void SetFinger(GamepadId gamepad, std::size_t touchpad, std::size_t finger, const GamepadFinger& state)
        {
            m_Readings.at(gamepad).State.Touchpads.at(touchpad).Fingers.at(finger) = state;
        }

        void SetCanRumble(bool canRumble) { m_CanRumble = canRumble; }

        const std::vector<RecordedRumble>& GetRumbles() const { return m_Rumbles; }

    private:
        class Backend final : public Quasar::GamepadBackend
        {
        public:
            explicit Backend(FakeGamepads& fake) : m_Fake(&fake) {}

            Quasar::GamepadReadings Read() override
            {
                Quasar::GamepadReadings readings{};
                for (const auto& [gamepad, reading] : m_Fake->m_Readings)
                {
                    readings.at(static_cast<std::size_t>(gamepad)) = reading;
                }
                return readings;
            }

            bool Rumble(GamepadId gamepad, const Quasar::GamepadRumble& rumble) override
            {
                m_Fake->m_Rumbles.push_back({.Gamepad = gamepad, .Rumble = rumble});
                return m_Fake->m_CanRumble;
            }

        private:
            FakeGamepads* m_Fake;
        };

        std::map<GamepadId, Quasar::GamepadReading> m_Readings;
        std::vector<RecordedRumble> m_Rumbles;
        bool m_CanRumble = true;
    };

    Quasar::Gamepads CreateGamepads(FakeGamepads& fake)
    {
        Quasar::Gamepads gamepads(fake.CreateBackend());
        gamepads.GetSettings() = TestSettings;
        return gamepads;
    }

    class GamepadLayer : public Quasar::Layer
    {
    public:
        explicit GamepadLayer(FakeGamepads& fake) : Layer("Gamepad"), m_Fake(&fake) {}

        void OnUpdate(Quasar::Timestep /*timestep*/) override
        {
            ++m_Updates;
            if (m_Updates == 2)
            {
                m_ConnectedInSecondFrame = Quasar::Input::IsGamepadConnected(First);
                m_APressedInSecondFrame = Quasar::Input::IsGamepadButtonPressed(First, GamepadButton::A);
                m_Fake->SetButton(First, GamepadButton::B, true);
            }
            if (m_Updates == 3)
            {
                Quasar::Application::Get().Close();
            }
        }

        void OnEvent(Quasar::Event& event) override { Record(event, m_Events); }

        const std::vector<RecordedEvent>& GetEvents() const { return m_Events; }
        bool WasConnectedInSecondFrame() const { return m_ConnectedInSecondFrame; }
        bool WasAPressedInSecondFrame() const { return m_APressedInSecondFrame; }

    private:
        FakeGamepads* m_Fake;
        int m_Updates = 0;
        std::vector<RecordedEvent> m_Events;
        bool m_ConnectedInSecondFrame = false;
        bool m_APressedInSecondFrame = false;
    };

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

TEST_CASE("A connected gamepad sends a connected event and no event for the buttons already held down")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First, "Xbox Controller");
    fake.SetButton(First, GamepadButton::A, true);

    const std::vector<RecordedEvent> events = Update(gamepads);

    CHECK(events == std::vector<RecordedEvent>{Connected(First)});
    CHECK(gamepads.IsConnected(First));
    CHECK(gamepads.GetName(First) == "Xbox Controller");
    CHECK(gamepads.IsButtonPressed(First, GamepadButton::A));
}

TEST_CASE("A disconnected gamepad releases its buttons before it sends a disconnected event")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    Update(gamepads);
    fake.SetButton(First, GamepadButton::A, true);
    fake.SetButton(First, GamepadButton::DPadUp, true);
    Update(gamepads);

    fake.Disconnect(First);
    const std::vector<RecordedEvent> events = Update(gamepads);

    const std::vector<RecordedEvent> expected{Released(First, GamepadButton::A), Released(First, GamepadButton::DPadUp), Disconnected(First)};
    CHECK(events == expected);
    CHECK_FALSE(gamepads.IsConnected(First));
    CHECK_FALSE(gamepads.IsButtonPressed(First, GamepadButton::A));
    CHECK(gamepads.GetName(First).empty());
    CHECK(Update(gamepads).empty());
}

TEST_CASE("Each gamepad sends the events of its own id")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    fake.Connect(Second);
    CHECK(Update(gamepads) == std::vector<RecordedEvent>{Connected(First), Connected(Second)});

    fake.SetButton(Second, GamepadButton::B, true);

    CHECK(Update(gamepads) == std::vector<RecordedEvent>{Pressed(Second, GamepadButton::B)});
    CHECK_FALSE(gamepads.IsButtonPressed(First, GamepadButton::B));
    CHECK(gamepads.IsButtonPressed(Second, GamepadButton::B));
}

TEST_CASE("A gamepad id that cannot exist is never connected")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    const auto outOfRange = static_cast<GamepadId>(Quasar::MaxGamepads);

    Update(gamepads);

    CHECK_FALSE(gamepads.IsConnected(outOfRange));
    CHECK_FALSE(gamepads.IsButtonPressed(outOfRange, GamepadButton::A));
    CHECK(gamepads.GetAxis(outOfRange, GamepadAxis::LeftX) == 0.0f);
}

TEST_CASE("Pressing and releasing a button sends one event each")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    Update(gamepads);

    fake.SetButton(First, GamepadButton::X, true);
    CHECK(Update(gamepads) == std::vector<RecordedEvent>{Pressed(First, GamepadButton::X)});
    CHECK(gamepads.IsButtonPressed(First, GamepadButton::X));

    CHECK(Update(gamepads).empty());

    fake.SetButton(First, GamepadButton::X, false);
    CHECK(Update(gamepads) == std::vector<RecordedEvent>{Released(First, GamepadButton::X)});
    CHECK_FALSE(gamepads.IsButtonPressed(First, GamepadButton::X));
}

TEST_CASE("A stick inside its dead zone reads as zero and sends no event")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    Update(gamepads);

    fake.SetAxis(First, GamepadAxis::LeftX, 0.1f);
    fake.SetAxis(First, GamepadAxis::LeftY, -0.05f);

    CHECK(Update(gamepads).empty());
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftX) == 0.0f);
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftY) == 0.0f);
}

TEST_CASE("The dead zone of a stick is a circle, so a diagonal stays a diagonal")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    Update(gamepads);

    fake.SetAxis(First, GamepadAxis::RightX, 0.5f);
    fake.SetAxis(First, GamepadAxis::RightY, 0.5f);
    const std::vector<RecordedEvent> events = Update(gamepads);

    const float x = gamepads.GetAxis(First, GamepadAxis::RightX);
    const float y = gamepads.GetAxis(First, GamepadAxis::RightY);
    CHECK(x == y);
    // The distance 0.707 past the dead zone of 0.15 is scaled to (0.707 - 0.15) / 0.85, shared by both axes
    CHECK(x == doctest::Approx(0.4635).epsilon(0.001));
    REQUIRE(events.size() == 2);
    CHECK(events[0].Axis == GamepadAxis::RightX);
    CHECK(events[1].Axis == GamepadAxis::RightY);
}

TEST_CASE("A stick pushed to the end reads as 1")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    Update(gamepads);

    fake.SetAxis(First, GamepadAxis::LeftX, -1.0f);
    Update(gamepads);

    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftX) == -1.0f);
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftY) == 0.0f);
}

TEST_CASE("A trigger reads from 0 at rest to 1 when fully pressed")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    Update(gamepads);
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftTrigger) == 0.0f);

    fake.SetAxis(First, GamepadAxis::LeftTrigger, 0.5f);
    Update(gamepads);
    // Halfway is 0.5, then scaled past the dead zone of 0.05
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftTrigger) == doctest::Approx(0.4737).epsilon(0.001));

    fake.SetAxis(First, GamepadAxis::LeftTrigger, 1.0f);
    Update(gamepads);
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftTrigger) == 1.0f);
}

TEST_CASE("Small axis changes send no event, but returning to rest does")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    Update(gamepads);
    fake.SetAxis(First, GamepadAxis::LeftX, 0.6f);
    REQUIRE(Update(gamepads).size() == 1);

    fake.SetAxis(First, GamepadAxis::LeftX, 0.603f);
    CHECK(Update(gamepads).empty());
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftX) > 0.0f);

    fake.SetAxis(First, GamepadAxis::LeftX, 0.0f);
    const std::vector<RecordedEvent> events = Update(gamepads);
    REQUIRE(events.size() == 1);
    CHECK(events[0].Type == EventType::GamepadAxisMoved);
    CHECK(events[0].Value == 0.0f);
}

TEST_CASE("The dead zones can be changed in the settings")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    gamepads.GetSettings().StickDeadZone = 0.0f;
    fake.Connect(First);
    fake.SetAxis(First, GamepadAxis::LeftX, 0.1f);

    Update(gamepads);

    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftX) == doctest::Approx(0.1));
}

TEST_CASE("A connected gamepad keeps its type until it is disconnected")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First, "DualSense", GamepadType::PlayStation5);
    Update(gamepads);

    CHECK(gamepads.GetType(First) == GamepadType::PlayStation5);

    fake.Disconnect(First);
    Update(gamepads);

    CHECK(gamepads.GetType(First) == GamepadType::Unknown);
}

TEST_CASE("A rumble only reaches a connected gamepad")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    const Quasar::GamepadRumble rumble{.LowFrequency = 0.25f, .HighFrequency = 0.75f, .Duration = std::chrono::milliseconds(500)};

    CHECK_FALSE(gamepads.Rumble(First, rumble));
    CHECK(fake.GetRumbles().empty());

    fake.Connect(First);
    Update(gamepads);

    CHECK(gamepads.Rumble(First, rumble));
    CHECK_FALSE(gamepads.Rumble(Second, rumble));
    REQUIRE(fake.GetRumbles().size() == 1);
    CHECK(fake.GetRumbles()[0].Gamepad == First);
    CHECK(fake.GetRumbles()[0].Rumble.LowFrequency == 0.25f);
    CHECK(fake.GetRumbles()[0].Rumble.HighFrequency == 0.75f);
    CHECK(fake.GetRumbles()[0].Rumble.Duration == std::chrono::milliseconds(500));
}

TEST_CASE("A rumble fails on a gamepad that cannot rumble and without a backend")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    fake.SetCanRumble(false);
    Update(gamepads);
    const Quasar::GamepadRumble rumble{.LowFrequency = 1.0f, .HighFrequency = 1.0f, .Duration = std::chrono::milliseconds(100)};

    CHECK_FALSE(gamepads.Rumble(First, rumble));

    gamepads.SetBackend(nullptr);
    Update(gamepads);

    CHECK_FALSE(gamepads.IsConnected(First));
    CHECK_FALSE(gamepads.Rumble(First, rumble));
}

TEST_CASE("A finger on a touchpad sends a down event, a moved event for each change, and an up event where it left")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    fake.SetTouchpadCount(First, 2);
    Update(gamepads);
    CHECK(gamepads.GetTouchpadCount(First) == 2);

    const GamepadFinger touching{.Down = true, .Position = {0.25f, 0.5f}, .Pressure = 0.5f};
    fake.SetFinger(First, 1, 0, touching);
    CHECK(Update(gamepads) == std::vector<RecordedEvent>{FingerDown(First, 1, 0, {0.25f, 0.5f}, 0.5f)});
    CHECK(gamepads.GetTouchpadFinger(First, 1, 0) == touching);
    CHECK(Update(gamepads).empty());

    fake.SetFinger(First, 1, 0, {.Down = true, .Position = {0.75f, 0.5f}, .Pressure = 0.5f});
    CHECK(Update(gamepads) == std::vector<RecordedEvent>{FingerMoved(First, 1, 0, {0.75f, 0.5f}, 0.5f)});

    fake.SetFinger(First, 1, 0, {.Down = true, .Position = {0.75f, 0.5f}, .Pressure = 1.0f});
    CHECK(Update(gamepads) == std::vector<RecordedEvent>{FingerMoved(First, 1, 0, {0.75f, 0.5f}, 1.0f)});

    fake.SetFinger(First, 1, 0, {});
    CHECK(Update(gamepads) == std::vector<RecordedEvent>{FingerUp(First, 1, 0, {0.75f, 0.5f})});
    CHECK_FALSE(gamepads.GetTouchpadFinger(First, 1, 0).Down);
}

TEST_CASE("Each finger on a touchpad sends the events of its own index")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    fake.SetTouchpadCount(First, 1);
    Update(gamepads);

    fake.SetFinger(First, 0, 0, {.Down = true, .Position = {0.1f, 0.1f}, .Pressure = 1.0f});
    fake.SetFinger(First, 0, 1, {.Down = true, .Position = {0.9f, 0.9f}, .Pressure = 1.0f});

    const std::vector<RecordedEvent> expected{FingerDown(First, 0, 0, {0.1f, 0.1f}, 1.0f), FingerDown(First, 0, 1, {0.9f, 0.9f}, 1.0f)};
    CHECK(Update(gamepads) == expected);
}

TEST_CASE("A finger already on a touchpad when the gamepad is connected sends no event")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    fake.SetTouchpadCount(First, 1);
    fake.SetFinger(First, 0, 0, {.Down = true, .Position = {0.5f, 0.5f}, .Pressure = 1.0f});

    CHECK(Update(gamepads) == std::vector<RecordedEvent>{Connected(First)});
    CHECK(gamepads.GetTouchpadFinger(First, 0, 0).Down);
}

TEST_CASE("A disconnected gamepad lifts its fingers after it releases its buttons")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    fake.SetTouchpadCount(First, 1);
    Update(gamepads);
    fake.SetButton(First, GamepadButton::Touchpad, true);
    fake.SetFinger(First, 0, 0, {.Down = true, .Position = {0.5f, 0.25f}, .Pressure = 1.0f});
    Update(gamepads);

    fake.Disconnect(First);
    const std::vector<RecordedEvent> events = Update(gamepads);

    const std::vector<RecordedEvent> expected{Released(First, GamepadButton::Touchpad), FingerUp(First, 0, 0, {0.5f, 0.25f}), Disconnected(First)};
    CHECK(events == expected);
    CHECK(gamepads.GetTouchpadCount(First) == 0);
    CHECK_FALSE(gamepads.GetTouchpadFinger(First, 0, 0).Down);
}

TEST_CASE("A touchpad or a finger that the gamepad does not have is never down")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    fake.SetTouchpadCount(First, 1);
    Update(gamepads);

    fake.SetFinger(First, 1, 0, {.Down = true, .Position = {0.5f, 0.5f}, .Pressure = 1.0f});

    CHECK(Update(gamepads).empty());
    CHECK_FALSE(gamepads.GetTouchpadFinger(First, 1, 0).Down);
    CHECK_FALSE(gamepads.GetTouchpadFinger(First, 0, Quasar::MaxGamepadTouchpadFingers).Down);
    CHECK_FALSE(gamepads.GetTouchpadFinger(First, Quasar::MaxGamepadTouchpads, 0).Down);
}

TEST_CASE("The application reads the gamepads each frame and sends their events to the layers")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);
    FakeGamepads fake;
    fake.Connect(First);
    fake.SetButton(First, GamepadButton::A, true);
    fake.SetAxis(First, GamepadAxis::LeftTrigger, 1.0f);
    fake.SetTouchpadCount(First, 1);
    fake.SetFinger(First, 0, 0, {.Down = true, .Position = {0.5f, 0.5f}, .Pressure = 1.0f});
    application->GetGamepads().SetBackend(fake.CreateBackend());
    auto layer = Quasar::CreateScope<GamepadLayer>(fake);
    const GamepadLayer& gamepadLayer = *layer;
    application->PushLayer(std::move(layer));

    application->Run();

    CHECK(gamepadLayer.WasConnectedInSecondFrame());
    CHECK(gamepadLayer.WasAPressedInSecondFrame());
    CHECK(gamepadLayer.GetEvents() == std::vector<RecordedEvent>{Connected(First), Pressed(First, GamepadButton::B)});
    CHECK(Quasar::Input::IsGamepadButtonPressed(First, GamepadButton::B));
    CHECK(Quasar::Input::GetGamepadAxis(First, GamepadAxis::LeftTrigger) == 1.0f);
    CHECK(Quasar::Input::GetGamepadTouchpadFinger(First, 0, 0).Down);
}

TEST_CASE("A gamepad read with SDL is connected with its name")
{
    const TestHelpers::ScopedLog log;
    Quasar::Gamepads gamepads;
    const VirtualGamepad virtualGamepad;
    REQUIRE(virtualGamepad.IsPlugged());

    const std::vector<RecordedEvent> events = Update(gamepads);

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
    Update(gamepads);
    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());
    CHECK(gamepads.GetAxis(*gamepad, GamepadAxis::LeftTrigger) == 0.0f);

    virtualGamepad.SetButton(GamepadButton::LeftBumper, true);
    virtualGamepad.SetButton(GamepadButton::DPadLeft, true);
    virtualGamepad.SetAxis(GamepadAxis::LeftY, SDL_JOYSTICK_AXIS_MIN);
    virtualGamepad.SetAxis(GamepadAxis::RightX, SDL_JOYSTICK_AXIS_MAX);
    virtualGamepad.SetAxis(GamepadAxis::RightTrigger, SDL_JOYSTICK_AXIS_MAX);
    Update(gamepads);

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
    Update(gamepads);
    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());

    virtualGamepad.SetButton(GamepadButton::RightPaddle1, true);
    virtualGamepad.SetButton(GamepadButton::Touchpad, true);
    virtualGamepad.SetButton(GamepadButton::Misc6, true);
    const std::vector<RecordedEvent> events = Update(gamepads);

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
    Update(gamepads);
    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());
    REQUIRE(gamepads.GetType(*gamepad) == GamepadType::SteamController);

    virtualGamepad.SetButton(GamepadButton::Misc1, true);
    virtualGamepad.SetButton(GamepadButton::Misc3, true);
    virtualGamepad.SetButton(GamepadButton::Misc6, true);
    const std::vector<RecordedEvent> events = Update(gamepads);

    CHECK(events == std::vector<RecordedEvent>{Pressed(*gamepad, GamepadButton::Misc1)});
    CHECK_FALSE(gamepads.IsButtonPressed(*gamepad, GamepadButton::Misc3));
    CHECK_FALSE(gamepads.IsButtonPressed(*gamepad, GamepadButton::Misc6));
    CHECK(gamepads.IsTouched(*gamepad, GamepadTouchSensor::LeftStick));
    CHECK(gamepads.IsTouched(*gamepad, GamepadTouchSensor::RightGrip));
    CHECK_FALSE(gamepads.IsTouched(*gamepad, GamepadTouchSensor::RightStick));
    CHECK_FALSE(gamepads.IsTouched(*gamepad, GamepadTouchSensor::LeftGrip));

    virtualGamepad.SetButton(GamepadButton::Misc3, false);
    Update(gamepads);

    CHECK_FALSE(gamepads.IsTouched(*gamepad, GamepadTouchSensor::LeftStick));
}

TEST_CASE("A gamepad that is not a Steam controller has no touch sensor")
{
    Quasar::Gamepads gamepads;
    VirtualGamepad virtualGamepad;
    REQUIRE(virtualGamepad.IsPlugged());
    Update(gamepads);
    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());

    virtualGamepad.SetButton(GamepadButton::Misc3, true);
    Update(gamepads);

    CHECK(gamepads.IsButtonPressed(*gamepad, GamepadButton::Misc3));
    CHECK_FALSE(gamepads.IsTouched(*gamepad, GamepadTouchSensor::LeftStick));
}

TEST_CASE("The fingers on the touchpads of a gamepad read with SDL are at their place")
{
    Quasar::Gamepads gamepads;
    VirtualGamepad virtualGamepad;
    REQUIRE(virtualGamepad.IsPlugged());
    Update(gamepads);
    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());
    CHECK(gamepads.GetTouchpadCount(*gamepad) == VirtualTouchpadCount);

    const GamepadFinger touching{.Down = true, .Position = {0.25f, 0.75f}, .Pressure = 0.5f};
    virtualGamepad.SetFinger(1, 1, touching);
    CHECK(Update(gamepads) == std::vector<RecordedEvent>{FingerDown(*gamepad, 1, 1, {0.25f, 0.75f}, 0.5f)});
    CHECK(gamepads.GetTouchpadFinger(*gamepad, 1, 1) == touching);
    CHECK_FALSE(gamepads.GetTouchpadFinger(*gamepad, 0, 0).Down);

    virtualGamepad.SetFinger(1, 1, {});
    CHECK(Update(gamepads) == std::vector<RecordedEvent>{FingerUp(*gamepad, 1, 1, {0.25f, 0.75f})});
}

TEST_CASE("A gamepad read with SDL releases its buttons and is disconnected when it is unplugged")
{
    Quasar::Gamepads gamepads;
    VirtualGamepad virtualGamepad;
    REQUIRE(virtualGamepad.IsPlugged());
    virtualGamepad.SetButton(GamepadButton::Y, true);
    Update(gamepads);
    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());

    virtualGamepad.Unplug();
    const std::vector<RecordedEvent> events = Update(gamepads);

    CHECK_FALSE(gamepads.IsConnected(*gamepad));
    CHECK(events == std::vector<RecordedEvent>{Released(*gamepad, GamepadButton::Y), Disconnected(*gamepad)});
}

TEST_CASE("A gamepad read with SDL never takes the id of a gamepad unplugged in the same frame")
{
    Quasar::Gamepads gamepads;
    VirtualGamepad first;
    REQUIRE(first.IsPlugged());
    Update(gamepads);
    const std::optional<GamepadId> firstId = FindVirtualGamepad(gamepads);
    REQUIRE(firstId.has_value());

    first.Unplug();
    const VirtualGamepad second;
    REQUIRE(second.IsPlugged());
    const std::vector<RecordedEvent> events = Update(gamepads);

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
    Update(gamepads);
    const std::optional<GamepadId> firstId = FindVirtualGamepad(gamepads);
    REQUIRE(firstId.has_value());

    virtualGamepads.front()->Unplug();
    const std::vector<RecordedEvent> unplugEvents = Update(gamepads);
    const std::vector<RecordedEvent> nextEvents = Update(gamepads);

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

    Update(gamepads);

    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());
    CHECK(gamepads.GetType(*gamepad) == GamepadType::PlayStation5);
}

TEST_CASE("A rumble of a gamepad read with SDL reaches its motors with the right strength")
{
    Quasar::Gamepads gamepads;
    const VirtualGamepad virtualGamepad;
    REQUIRE(virtualGamepad.IsPlugged());
    Update(gamepads);
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
    Update(gamepads);
    const std::optional<GamepadId> gamepad = FindVirtualGamepad(gamepads);
    REQUIRE(gamepad.has_value());

    REQUIRE(gamepads.Rumble(*gamepad, {.LowFrequency = 1.0f, .HighFrequency = 1.0f, .Duration = std::chrono::milliseconds(1)}));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    Update(gamepads);

    REQUIRE_FALSE(virtualGamepad.GetRumbles().empty());
    CHECK(virtualGamepad.GetRumbles().back() == MotorStrengths{});
}
