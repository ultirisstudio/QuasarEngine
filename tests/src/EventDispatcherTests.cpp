#include <quasar/events/Event.h>
#include <quasar/events/EventDispatcher.h>

#include <doctest/doctest.h>

using Quasar::EventCategory;
using Quasar::EventDispatcher;
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

        int GetKey() const { return m_Key; }

    private:
        int m_Key = 0;
    };

    static_assert(Quasar::EventClass<TestKeyEvent>);
    static_assert(!Quasar::EventClass<Quasar::Event>, "Event has no static type, so it cannot be dispatched");
    static_assert(!Quasar::EventClass<int>);
}

TEST_CASE("A handler is only called for events of its type")
{
    TestKeyEvent event(42);
    EventDispatcher dispatcher(event);
    bool closeHandlerCalled = false;
    bool keyHandlerCalled = false;

    const bool closeDispatched = dispatcher.Dispatch<TestCloseEvent>(
        [&](TestCloseEvent&)
        {
            closeHandlerCalled = true;
            return true;
        });
    const bool keyDispatched = dispatcher.Dispatch<TestKeyEvent>(
        [&](TestKeyEvent&)
        {
            keyHandlerCalled = true;
            return false;
        });

    CHECK_FALSE(closeDispatched);
    CHECK_FALSE(closeHandlerCalled);
    CHECK(keyDispatched);
    CHECK(keyHandlerCalled);
}

TEST_CASE("The handler receives the event with its data")
{
    TestKeyEvent event(42);
    EventDispatcher dispatcher(event);
    int receivedKey = 0;

    dispatcher.Dispatch<TestKeyEvent>(
        [&](TestKeyEvent& key)
        {
            receivedKey = key.GetKey();
            return true;
        });

    CHECK(receivedKey == 42);
}

TEST_CASE("An event is handled when its handler returns true")
{
    TestCloseEvent event;
    EventDispatcher dispatcher(event);

    dispatcher.Dispatch<TestCloseEvent>([](TestCloseEvent&) { return true; });

    CHECK(event.IsHandled());
}

TEST_CASE("An event is not handled when its handler returns false")
{
    TestCloseEvent event;
    EventDispatcher dispatcher(event);

    dispatcher.Dispatch<TestCloseEvent>([](TestCloseEvent&) { return false; });

    CHECK_FALSE(event.IsHandled());
}

TEST_CASE("A handled event stays handled when a later handler returns false")
{
    TestCloseEvent event;
    EventDispatcher dispatcher(event);

    dispatcher.Dispatch<TestCloseEvent>([](TestCloseEvent&) { return true; });
    dispatcher.Dispatch<TestCloseEvent>([](TestCloseEvent&) { return false; });

    CHECK(event.IsHandled());
}

TEST_CASE("A handler that does not match the type leaves the event unchanged")
{
    TestKeyEvent event(42);
    EventDispatcher dispatcher(event);

    dispatcher.Dispatch<TestCloseEvent>([](TestCloseEvent&) { return true; });

    CHECK_FALSE(event.IsHandled());
}