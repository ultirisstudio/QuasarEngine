#include <quasar/events/Event.h>

#include <doctest/doctest.h>
#include <spdlog/fmt/fmt.h>

#include <string>
#include <type_traits>

using Quasar::EventCategory;
using Quasar::EventType;

namespace
{
    class TestCloseEvent : public Quasar::EventOf<EventType::WindowClose, EventCategory::Window>
    {
    };

    class TestKeyEvent : public Quasar::EventOf<EventType::KeyPressed, EventCategory::Input | EventCategory::Keyboard>
    {
    public:
        explicit TestKeyEvent(int key) : m_Key(key) {}

        std::string ToString() const override { return fmt::format("{}: {}", GetName(), m_Key); }

    private:
        int m_Key = 0;
    };

    static_assert(TestKeyEvent::GetStaticType() == EventType::KeyPressed);
    static_assert(std::is_copy_constructible_v<TestKeyEvent>, "Events must be copyable to be queued later");
    static_assert(!std::is_copy_constructible_v<Quasar::Event>, "Copying an Event directly would slice it");
}

TEST_CASE("An event exposes its type and its categories")
{
    const TestKeyEvent event(42);
    const Quasar::Event& base = event;

    CHECK(base.GetType() == EventType::KeyPressed);
    CHECK(base.GetCategories() == (EventCategory::Input | EventCategory::Keyboard));
    CHECK(base.IsInCategory(EventCategory::Input));
    CHECK(base.IsInCategory(EventCategory::Keyboard));
    CHECK_FALSE(base.IsInCategory(EventCategory::Mouse));
    CHECK_FALSE(base.IsInCategory(EventCategory::Window));
}

TEST_CASE("An event is not handled until it is marked as handled")
{
    TestCloseEvent event;
    CHECK_FALSE(event.IsHandled());

    event.SetHandled(true);
    CHECK(event.IsHandled());
}

TEST_CASE("An event without data is printed with its name")
{
    const TestCloseEvent event;

    CHECK(event.GetName() == std::string("WindowClose"));
    CHECK(event.ToString() == "WindowClose");
}

TEST_CASE("An event is printed by the log macros with its data")
{
    const TestKeyEvent event(42);
    const Quasar::Event& base = event;

    CHECK(fmt::format("{}", event) == "KeyPressed: 42");
    CHECK(fmt::format("{}", base) == "KeyPressed: 42");
}

TEST_CASE("Every event type has a name")
{
    CHECK(Quasar::GetEventTypeName(EventType::MouseScrolled) == std::string("MouseScrolled"));
    CHECK(Quasar::GetEventTypeName(EventType::WindowFocus) == std::string("WindowFocus"));
}
