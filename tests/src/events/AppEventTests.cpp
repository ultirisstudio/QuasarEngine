#include <quasar/events/AppEvent.h>

#include <doctest/doctest.h>

#include <array>

using Quasar::AppBackgroundEvent;
using Quasar::AppLowMemoryEvent;
using Quasar::AppTerminatingEvent;
using Quasar::EventCategory;
using Quasar::EventType;

TEST_CASE("Application events have their own type")
{
    CHECK(AppTerminatingEvent().GetType() == EventType::AppTerminating);
    CHECK(AppLowMemoryEvent().GetType() == EventType::AppLowMemory);
    CHECK(AppBackgroundEvent(true).GetType() == EventType::AppBackground);
}

TEST_CASE("All application events are only in the application category")
{
    const AppTerminatingEvent terminating;
    const AppLowMemoryEvent lowMemory;
    const AppBackgroundEvent background(true);

    const auto events = std::to_array<const Quasar::Event*>({&terminating, &lowMemory, &background});

    for (const Quasar::Event* event : events)
    {
        CHECK(event->IsInCategory(EventCategory::Application));
        CHECK_FALSE(event->IsInCategory(EventCategory::Window));
        CHECK_FALSE(event->IsInCategory(EventCategory::Input));
    }
}

TEST_CASE("A background event tells whether the application went to the background or came back")
{
    const AppBackgroundEvent background(true);
    const AppBackgroundEvent foreground(false);

    CHECK(background.IsInBackground());
    CHECK_FALSE(foreground.IsInBackground());
}
