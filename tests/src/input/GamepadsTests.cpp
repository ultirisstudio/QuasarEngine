#include "helpers/GamepadEvents.h"
#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Application.h>
#include <quasar/core/Layer.h>
#include <quasar/events/Event.h>
#include <quasar/input/GamepadBackend.h>
#include <quasar/input/GamepadCodes.h>
#include <quasar/input/GamepadState.h>
#include <quasar/input/Gamepads.h>
#include <quasar/input/Input.h>

#include <doctest/doctest.h>
#include <glm/vec2.hpp>

#include <chrono>
#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

using Quasar::EventType;
using Quasar::GamepadAxis;
using Quasar::GamepadButton;
using Quasar::GamepadFinger;
using Quasar::GamepadId;
using Quasar::GamepadType;

using TestHelpers::Connected;
using TestHelpers::Disconnected;
using TestHelpers::FingerDown;
using TestHelpers::FingerMoved;
using TestHelpers::FingerUp;
using TestHelpers::Pressed;
using TestHelpers::RecordedEvent;
using TestHelpers::Released;
using TestHelpers::UpdateGamepads;

namespace
{
    constexpr GamepadId First{0};
    constexpr GamepadId Second{1};

    constexpr Quasar::GamepadSettings TestSettings{.StickDeadZone = 0.15f, .TriggerDeadZone = 0.05f, .AxisEventThreshold = 0.01f};

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

        void OnEvent(Quasar::Event& event) override { TestHelpers::RecordGamepadEvent(event, m_Events); }

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
}

TEST_CASE("A connected gamepad sends a connected event and no event for the buttons already held down")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First, "Xbox Controller");
    fake.SetButton(First, GamepadButton::A, true);

    const std::vector<RecordedEvent> events = UpdateGamepads(gamepads);

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
    UpdateGamepads(gamepads);
    fake.SetButton(First, GamepadButton::A, true);
    fake.SetButton(First, GamepadButton::DPadUp, true);
    UpdateGamepads(gamepads);

    fake.Disconnect(First);
    const std::vector<RecordedEvent> events = UpdateGamepads(gamepads);

    const std::vector<RecordedEvent> expected{Released(First, GamepadButton::A), Released(First, GamepadButton::DPadUp), Disconnected(First)};
    CHECK(events == expected);
    CHECK_FALSE(gamepads.IsConnected(First));
    CHECK_FALSE(gamepads.IsButtonPressed(First, GamepadButton::A));
    CHECK(gamepads.GetName(First).empty());
    CHECK(UpdateGamepads(gamepads).empty());
}

TEST_CASE("Each gamepad sends the events of its own id")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    fake.Connect(Second);
    CHECK(UpdateGamepads(gamepads) == std::vector<RecordedEvent>{Connected(First), Connected(Second)});

    fake.SetButton(Second, GamepadButton::B, true);

    CHECK(UpdateGamepads(gamepads) == std::vector<RecordedEvent>{Pressed(Second, GamepadButton::B)});
    CHECK_FALSE(gamepads.IsButtonPressed(First, GamepadButton::B));
    CHECK(gamepads.IsButtonPressed(Second, GamepadButton::B));
}

TEST_CASE("A gamepad id that cannot exist is never connected")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    const auto outOfRange = static_cast<GamepadId>(Quasar::MaxGamepads);

    UpdateGamepads(gamepads);

    CHECK_FALSE(gamepads.IsConnected(outOfRange));
    CHECK_FALSE(gamepads.IsButtonPressed(outOfRange, GamepadButton::A));
    CHECK(gamepads.GetAxis(outOfRange, GamepadAxis::LeftX) == 0.0f);
}

TEST_CASE("Pressing and releasing a button sends one event each")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    UpdateGamepads(gamepads);

    fake.SetButton(First, GamepadButton::X, true);
    CHECK(UpdateGamepads(gamepads) == std::vector<RecordedEvent>{Pressed(First, GamepadButton::X)});
    CHECK(gamepads.IsButtonPressed(First, GamepadButton::X));

    CHECK(UpdateGamepads(gamepads).empty());

    fake.SetButton(First, GamepadButton::X, false);
    CHECK(UpdateGamepads(gamepads) == std::vector<RecordedEvent>{Released(First, GamepadButton::X)});
    CHECK_FALSE(gamepads.IsButtonPressed(First, GamepadButton::X));
}

TEST_CASE("A stick inside its dead zone reads as zero and sends no event")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    UpdateGamepads(gamepads);

    fake.SetAxis(First, GamepadAxis::LeftX, 0.1f);
    fake.SetAxis(First, GamepadAxis::LeftY, -0.05f);

    CHECK(UpdateGamepads(gamepads).empty());
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftX) == 0.0f);
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftY) == 0.0f);
}

TEST_CASE("The dead zone of a stick is a circle, so a diagonal stays a diagonal")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    UpdateGamepads(gamepads);

    fake.SetAxis(First, GamepadAxis::RightX, 0.5f);
    fake.SetAxis(First, GamepadAxis::RightY, 0.5f);
    const std::vector<RecordedEvent> events = UpdateGamepads(gamepads);

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
    UpdateGamepads(gamepads);

    fake.SetAxis(First, GamepadAxis::LeftX, -1.0f);
    UpdateGamepads(gamepads);

    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftX) == -1.0f);
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftY) == 0.0f);
}

TEST_CASE("A trigger reads from 0 at rest to 1 when fully pressed")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    UpdateGamepads(gamepads);
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftTrigger) == 0.0f);

    fake.SetAxis(First, GamepadAxis::LeftTrigger, 0.5f);
    UpdateGamepads(gamepads);
    // Halfway is 0.5, then scaled past the dead zone of 0.05
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftTrigger) == doctest::Approx(0.4737).epsilon(0.001));

    fake.SetAxis(First, GamepadAxis::LeftTrigger, 1.0f);
    UpdateGamepads(gamepads);
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftTrigger) == 1.0f);
}

TEST_CASE("Small axis changes send no event, but returning to rest does")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    UpdateGamepads(gamepads);
    fake.SetAxis(First, GamepadAxis::LeftX, 0.6f);
    REQUIRE(UpdateGamepads(gamepads).size() == 1);

    fake.SetAxis(First, GamepadAxis::LeftX, 0.603f);
    CHECK(UpdateGamepads(gamepads).empty());
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftX) > 0.0f);

    fake.SetAxis(First, GamepadAxis::LeftX, 0.0f);
    const std::vector<RecordedEvent> events = UpdateGamepads(gamepads);
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

    UpdateGamepads(gamepads);

    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftX) == doctest::Approx(0.1));
}

TEST_CASE("A connected gamepad keeps its type until it is disconnected")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First, "DualSense", GamepadType::PlayStation5);
    UpdateGamepads(gamepads);

    CHECK(gamepads.GetType(First) == GamepadType::PlayStation5);

    fake.Disconnect(First);
    UpdateGamepads(gamepads);

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
    UpdateGamepads(gamepads);

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
    UpdateGamepads(gamepads);
    const Quasar::GamepadRumble rumble{.LowFrequency = 1.0f, .HighFrequency = 1.0f, .Duration = std::chrono::milliseconds(100)};

    CHECK_FALSE(gamepads.Rumble(First, rumble));

    gamepads.SetBackend(nullptr);
    UpdateGamepads(gamepads);

    CHECK_FALSE(gamepads.IsConnected(First));
    CHECK_FALSE(gamepads.Rumble(First, rumble));
}

TEST_CASE("A finger on a touchpad sends a down event, a moved event for each change, and an up event where it left")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    fake.SetTouchpadCount(First, 2);
    UpdateGamepads(gamepads);
    CHECK(gamepads.GetTouchpadCount(First) == 2);

    const GamepadFinger touching{.Down = true, .Position = {0.25f, 0.5f}, .Pressure = 0.5f};
    fake.SetFinger(First, 1, 0, touching);
    CHECK(UpdateGamepads(gamepads) == std::vector<RecordedEvent>{FingerDown(First, 1, 0, {0.25f, 0.5f}, 0.5f)});
    CHECK(gamepads.GetTouchpadFinger(First, 1, 0) == touching);
    CHECK(UpdateGamepads(gamepads).empty());

    fake.SetFinger(First, 1, 0, {.Down = true, .Position = {0.75f, 0.5f}, .Pressure = 0.5f});
    CHECK(UpdateGamepads(gamepads) == std::vector<RecordedEvent>{FingerMoved(First, 1, 0, {0.75f, 0.5f}, 0.5f)});

    fake.SetFinger(First, 1, 0, {.Down = true, .Position = {0.75f, 0.5f}, .Pressure = 1.0f});
    CHECK(UpdateGamepads(gamepads) == std::vector<RecordedEvent>{FingerMoved(First, 1, 0, {0.75f, 0.5f}, 1.0f)});

    fake.SetFinger(First, 1, 0, {});
    CHECK(UpdateGamepads(gamepads) == std::vector<RecordedEvent>{FingerUp(First, 1, 0, {0.75f, 0.5f})});
    CHECK_FALSE(gamepads.GetTouchpadFinger(First, 1, 0).Down);
}

TEST_CASE("Each finger on a touchpad sends the events of its own index")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    fake.SetTouchpadCount(First, 1);
    UpdateGamepads(gamepads);

    fake.SetFinger(First, 0, 0, {.Down = true, .Position = {0.1f, 0.1f}, .Pressure = 1.0f});
    fake.SetFinger(First, 0, 1, {.Down = true, .Position = {0.9f, 0.9f}, .Pressure = 1.0f});

    const std::vector<RecordedEvent> expected{FingerDown(First, 0, 0, {0.1f, 0.1f}, 1.0f), FingerDown(First, 0, 1, {0.9f, 0.9f}, 1.0f)};
    CHECK(UpdateGamepads(gamepads) == expected);
}

TEST_CASE("A finger already on a touchpad when the gamepad is connected sends no event")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    fake.SetTouchpadCount(First, 1);
    fake.SetFinger(First, 0, 0, {.Down = true, .Position = {0.5f, 0.5f}, .Pressure = 1.0f});

    CHECK(UpdateGamepads(gamepads) == std::vector<RecordedEvent>{Connected(First)});
    CHECK(gamepads.GetTouchpadFinger(First, 0, 0).Down);
}

TEST_CASE("A disconnected gamepad lifts its fingers after it releases its buttons")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads = CreateGamepads(fake);
    fake.Connect(First);
    fake.SetTouchpadCount(First, 1);
    UpdateGamepads(gamepads);
    fake.SetButton(First, GamepadButton::Touchpad, true);
    fake.SetFinger(First, 0, 0, {.Down = true, .Position = {0.5f, 0.25f}, .Pressure = 1.0f});
    UpdateGamepads(gamepads);

    fake.Disconnect(First);
    const std::vector<RecordedEvent> events = UpdateGamepads(gamepads);

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
    UpdateGamepads(gamepads);

    fake.SetFinger(First, 1, 0, {.Down = true, .Position = {0.5f, 0.5f}, .Pressure = 1.0f});

    CHECK(UpdateGamepads(gamepads).empty());
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
