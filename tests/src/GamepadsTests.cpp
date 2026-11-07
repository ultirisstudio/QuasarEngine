#include "LogHelpers.h"
#include "NullWindow.h"

#include <quasar/core/Application.h>
#include <quasar/core/GamepadCodes.h>
#include <quasar/core/Gamepads.h>
#include <quasar/core/Input.h>
#include <quasar/core/Layer.h>
#include <quasar/events/EventDispatcher.h>
#include <quasar/events/GamepadEvent.h>

#include <doctest/doctest.h>

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using Quasar::EventType;
using Quasar::GamepadAxis;
using Quasar::GamepadButton;
using Quasar::GamepadId;

namespace
{
    class FakeGamepads
    {
    public:
        Quasar::Gamepads::Reader GetReader()
        {
            return [this](GamepadId gamepad) -> std::optional<Quasar::GamepadReading>
            {
                const auto found = m_Readings.find(gamepad);
                if (found == m_Readings.end())
                {
                    return std::nullopt;
                }
                return found->second;
            };
        }

        void Connect(GamepadId gamepad, std::string name = "Test gamepad")
        {
            Quasar::GamepadReading reading;
            reading.Name = std::move(name);
            // Like GLFW, the raw value of a trigger at rest is -1
            reading.State.Axes.at(static_cast<std::size_t>(GamepadAxis::LeftTrigger)) = -1.0f;
            reading.State.Axes.at(static_cast<std::size_t>(GamepadAxis::RightTrigger)) = -1.0f;
            m_Readings[gamepad] = reading;
        }

        void Disconnect(GamepadId gamepad) { m_Readings.erase(gamepad); }

        void SetButton(GamepadId gamepad, GamepadButton button, bool pressed)
        {
            m_Readings.at(gamepad).State.Buttons.at(static_cast<std::size_t>(button)) = pressed;
        }

        void SetAxis(GamepadId gamepad, GamepadAxis axis, float raw) { m_Readings.at(gamepad).State.Axes.at(static_cast<std::size_t>(axis)) = raw; }

    private:
        std::map<GamepadId, Quasar::GamepadReading> m_Readings;
    };

    struct RecordedEvent
    {
        EventType Type{};
        GamepadId Gamepad{};
        GamepadButton Button{};
        GamepadAxis Axis{};
        float Value = 0.0f;

        bool operator==(const RecordedEvent&) const = default;
    };

    void Record(Quasar::Event& event, std::vector<RecordedEvent>& events)
    {
        Quasar::EventDispatcher dispatcher(event);
        dispatcher.Dispatch<Quasar::GamepadConnectedEvent>(
            [&events](const Quasar::GamepadConnectedEvent& connected)
            {
                events.push_back({.Type = EventType::GamepadConnected, .Gamepad = connected.GetGamepad()});
                return false;
            });
        dispatcher.Dispatch<Quasar::GamepadDisconnectedEvent>(
            [&events](const Quasar::GamepadDisconnectedEvent& disconnected)
            {
                events.push_back({.Type = EventType::GamepadDisconnected, .Gamepad = disconnected.GetGamepad()});
                return false;
            });
        dispatcher.Dispatch<Quasar::GamepadButtonPressedEvent>(
            [&events](const Quasar::GamepadButtonPressedEvent& pressed)
            {
                events.push_back({.Type = EventType::GamepadButtonPressed, .Gamepad = pressed.GetGamepad(), .Button = pressed.GetButton()});
                return false;
            });
        dispatcher.Dispatch<Quasar::GamepadButtonReleasedEvent>(
            [&events](const Quasar::GamepadButtonReleasedEvent& released)
            {
                events.push_back({.Type = EventType::GamepadButtonReleased, .Gamepad = released.GetGamepad(), .Button = released.GetButton()});
                return false;
            });
        dispatcher.Dispatch<Quasar::GamepadAxisMovedEvent>(
            [&events](const Quasar::GamepadAxisMovedEvent& moved)
            {
                events.push_back({.Type = EventType::GamepadAxisMoved, .Gamepad = moved.GetGamepad(), .Axis = moved.GetAxis(), .Value = moved.GetValue()});
                return false;
            });
    }

    std::vector<RecordedEvent> Update(Quasar::Gamepads& gamepads)
    {
        std::vector<RecordedEvent> events;
        gamepads.Update([&events](Quasar::Event& event) { Record(event, events); });
        return events;
    }

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

    constexpr GamepadId First{0};
    constexpr GamepadId Second{1};
}

TEST_CASE("A connected gamepad sends a connected event and no event for the buttons already held down")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads(fake.GetReader());
    fake.Connect(First, "Xbox Controller");
    fake.SetButton(First, GamepadButton::A, true);

    const std::vector<RecordedEvent> events = Update(gamepads);

    CHECK(events == std::vector<RecordedEvent>{Connected(First)});
    CHECK(gamepads.IsConnected(First));
    CHECK(gamepads.GetName(First) == "Xbox Controller");
    CHECK(gamepads.IsButtonPressed(First, GamepadButton::A));
}

TEST_CASE("Pressing and releasing a button sends one event each")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads(fake.GetReader());
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

TEST_CASE("A disconnected gamepad releases its buttons before it sends a disconnected event")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads(fake.GetReader());
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

TEST_CASE("A stick inside its dead zone reads as zero and sends no event")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads(fake.GetReader());
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
    Quasar::Gamepads gamepads(fake.GetReader());
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
    Quasar::Gamepads gamepads(fake.GetReader());
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
    Quasar::Gamepads gamepads(fake.GetReader());
    fake.Connect(First);
    Update(gamepads);
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftTrigger) == 0.0f);

    fake.SetAxis(First, GamepadAxis::LeftTrigger, 0.0f);
    Update(gamepads);
    // Halfway is 0.5 on 0..1, then scaled past the dead zone of 0.05
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftTrigger) == doctest::Approx(0.4737).epsilon(0.001));

    fake.SetAxis(First, GamepadAxis::LeftTrigger, 1.0f);
    Update(gamepads);
    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftTrigger) == 1.0f);
}

TEST_CASE("Small axis changes send no event, but returning to rest does")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads(fake.GetReader());
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
    Quasar::Gamepads gamepads(fake.GetReader());
    gamepads.GetSettings().StickDeadZone = 0.0f;
    fake.Connect(First);
    fake.SetAxis(First, GamepadAxis::LeftX, 0.1f);

    Update(gamepads);

    CHECK(gamepads.GetAxis(First, GamepadAxis::LeftX) == doctest::Approx(0.1));
}

TEST_CASE("Each gamepad sends the events of its own id")
{
    FakeGamepads fake;
    Quasar::Gamepads gamepads(fake.GetReader());
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
    Quasar::Gamepads gamepads(fake.GetReader());
    const auto outOfRange = static_cast<GamepadId>(Quasar::MaxGamepads);

    Update(gamepads);

    CHECK_FALSE(gamepads.IsConnected(outOfRange));
    CHECK_FALSE(gamepads.IsButtonPressed(outOfRange, GamepadButton::A));
    CHECK(gamepads.GetAxis(outOfRange, GamepadAxis::LeftX) == 0.0f);
}

namespace
{
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
}

TEST_CASE("The application reads the gamepads each frame and sends their events to the layers")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);
    FakeGamepads fake;
    fake.Connect(First);
    fake.SetButton(First, GamepadButton::A, true);
    application->GetGamepads().SetReader(fake.GetReader());
    auto layer = Quasar::CreateScope<GamepadLayer>(fake);
    const GamepadLayer& gamepadLayer = *layer;
    application->PushLayer(std::move(layer));

    application->Run();

    CHECK(gamepadLayer.WasConnectedInSecondFrame());
    CHECK(gamepadLayer.WasAPressedInSecondFrame());
    CHECK(gamepadLayer.GetEvents() == std::vector<RecordedEvent>{Connected(First), Pressed(First, GamepadButton::B)});
    CHECK(Quasar::Input::IsGamepadButtonPressed(First, GamepadButton::B));
}
