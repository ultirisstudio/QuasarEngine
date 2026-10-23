#include <quasar/events/EventDispatcher.h>
#include <quasar/events/KeyEvent.h>

#include <doctest/doctest.h>

using Quasar::EventCategory;
using Quasar::EventType;
using Quasar::Key;
using Quasar::KeyPressedEvent;
using Quasar::KeyReleasedEvent;
using Quasar::KeyTypedEvent;

TEST_CASE("Keyboard events have their own type")
{
    CHECK(KeyPressedEvent(Key::W).GetType() == EventType::KeyPressed);
    CHECK(KeyReleasedEvent(Key::W).GetType() == EventType::KeyReleased);
    CHECK(KeyTypedEvent(U'w').GetType() == EventType::KeyTyped);
}

TEST_CASE("All keyboard events share the keyboard and input categories")
{
    const KeyPressedEvent pressed(Key::W);
    const KeyReleasedEvent released(Key::W);
    const KeyTypedEvent typed(U'w');

    const Quasar::Event* events[] = {&pressed, &released, &typed};

    for (const Quasar::Event* event : events)
    {
        CHECK(event->IsInCategory(EventCategory::Keyboard));
        CHECK(event->IsInCategory(EventCategory::Input));
        CHECK_FALSE(event->IsInCategory(EventCategory::Mouse));
        CHECK_FALSE(event->IsInCategory(EventCategory::Window));
    }
}

TEST_CASE("Key events carry the key code")
{
    CHECK(KeyPressedEvent(Key::Escape).GetKey() == Key::Escape);
    CHECK(KeyReleasedEvent(Key::Space).GetKey() == Key::Space);
}

TEST_CASE("A key pressed event tells whether the key is repeating")
{
    const KeyPressedEvent first(Key::W);
    const KeyPressedEvent repeat(Key::W, true);

    CHECK_FALSE(first.IsRepeat());
    CHECK(repeat.IsRepeat());
    CHECK(first.ToString() == "KeyPressed: 87");
    CHECK(repeat.ToString() == "KeyPressed: 87 (repeat)");
    CHECK(KeyReleasedEvent(Key::W).ToString() == "KeyReleased: 87");
}

TEST_CASE("A character event carries the typed Unicode character")
{
    const KeyTypedEvent ascii(U'A');
    const KeyTypedEvent accent(U'\u00E9');

    CHECK(ascii.GetCharacter() == U'A');
    CHECK(accent.GetCharacter() == U'\u00E9');
    CHECK(ascii.ToString() == "KeyTyped: 'A' (U+0041)");
    CHECK(accent.ToString() == "KeyTyped: U+00E9");
}

TEST_CASE("A keyboard event reaches the handler of its type")
{
    KeyPressedEvent event(Key::Escape);
    Quasar::EventDispatcher dispatcher(event);
    Key receivedKey = Key::Space;

    dispatcher.Dispatch<KeyReleasedEvent>([](KeyReleasedEvent&) { return true; });
    dispatcher.Dispatch<KeyPressedEvent>(
        [&](KeyPressedEvent& pressed)
        {
            receivedKey = pressed.GetKey();
            return true;
        });

    CHECK(receivedKey == Key::Escape);
    CHECK(event.IsHandled());
}
