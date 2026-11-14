#include <quasar/events/TouchEvent.h>

#include <doctest/doctest.h>
#include <glm/vec2.hpp>

#include <array>

using Quasar::EventCategory;
using Quasar::EventType;
using Quasar::TouchDownEvent;
using Quasar::TouchMovedEvent;
using Quasar::TouchUpEvent;

TEST_CASE("Touch events have their own type")
{
    CHECK(TouchDownEvent(1, {10.0f, 20.0f}, 1.0f).GetType() == EventType::TouchDown);
    CHECK(TouchMovedEvent(1, {10.0f, 20.0f}, 1.0f).GetType() == EventType::TouchMoved);
    CHECK(TouchUpEvent(1, {10.0f, 20.0f}, 0.0f).GetType() == EventType::TouchUp);
}

TEST_CASE("All touch events share the touch and input categories")
{
    const TouchDownEvent down(1, {10.0f, 20.0f}, 1.0f);
    const TouchMovedEvent moved(1, {10.0f, 20.0f}, 1.0f);
    const TouchUpEvent up(1, {10.0f, 20.0f}, 0.0f);

    const auto events = std::to_array<const Quasar::Event*>({&down, &moved, &up});

    for (const Quasar::Event* event : events)
    {
        CHECK(event->IsInCategory(EventCategory::Touch));
        CHECK(event->IsInCategory(EventCategory::Input));
        CHECK_FALSE(event->IsInCategory(EventCategory::Mouse));
    }
}

TEST_CASE("Touch events carry the finger, the position and the pressure")
{
    const TouchMovedEvent event(7, {640.0f, 360.5f}, 0.25f);

    CHECK(event.GetFinger() == 7);
    CHECK(event.GetPosition() == glm::vec2(640.0f, 360.5f));
    CHECK(event.GetPressure() == doctest::Approx(0.25f));
}

TEST_CASE("Touch events are printed differently when their finger or their data differ")
{
    const TouchMovedEvent event(7, {640.0f, 360.5f}, 0.25f);

    CHECK(event.ToString() != TouchMovedEvent(8, {640.0f, 360.5f}, 0.25f).ToString());
    CHECK(event.ToString() != TouchMovedEvent(7, {641.0f, 360.5f}, 0.25f).ToString());
    CHECK(event.ToString() != TouchMovedEvent(7, {640.0f, 361.5f}, 0.25f).ToString());
    CHECK(event.ToString() != TouchMovedEvent(7, {640.0f, 360.5f}, 0.5f).ToString());
}
