#include <quasar/events/Event.h>

#include <doctest/doctest.h>
#include <spdlog/fmt/fmt.h>

#include <cstddef>
#include <set>
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

TEST_CASE("An event without data is printed with the name of its type")
{
    const TestCloseEvent event;

    CHECK(event.GetName() == std::string(Quasar::GetEventTypeName(EventType::WindowClose)));
    CHECK(event.ToString() == event.GetName());
}

TEST_CASE("The log macros print an event with its own ToString, even through a base reference")
{
    const TestKeyEvent event(42);
    const Quasar::Event& base = event;

    CHECK(fmt::format("{}", event) == event.ToString());
    CHECK(fmt::format("{}", base) == event.ToString());
}

TEST_CASE("Every event type has its own name")
{
    constexpr auto lastType = static_cast<std::size_t>(EventType::TouchUp);

    std::set<std::string> names;
    for (std::size_t index = 0; index <= lastType; ++index)
    {
        const std::string name = Quasar::GetEventTypeName(static_cast<EventType>(index));
        CHECK_FALSE(name.empty());
        names.insert(name);
    }

    CHECK(names.size() == lastType + 1);
}
