#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Application.h>
#include <quasar/core/Window.h>
#include <quasar/input/Input.h>
#include <quasar/input/KeyCodes.h>
#include <quasar/input/MouseCodes.h>

#include <doctest/doctest.h>
#include <glm/vec2.hpp>

TEST_CASE("Input reads the state of the main window")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);

    Quasar::Window& window = application->GetWindow();
    TestHelpers::SendMouseMotion(window, 12.0f, 34.0f);
    TestHelpers::SendKey(window, Quasar::Key::A, true);
    TestHelpers::SendMouseButton(window, Quasar::MouseButton::Left, true);
    window.Update();

    CHECK(Quasar::Input::GetMousePosition() == glm::vec2(12.0f, 34.0f));
    CHECK(Quasar::Input::IsKeyPressed(Quasar::Key::A));
    CHECK_FALSE(Quasar::Input::IsKeyPressed(Quasar::Key::B));
    CHECK(Quasar::Input::IsMouseButtonPressed(Quasar::MouseButton::Left));
    CHECK_FALSE(Quasar::Input::IsMouseButtonPressed(Quasar::MouseButton::Right));

    TestHelpers::SendKey(window, Quasar::Key::A, false);
    TestHelpers::SendMouseButton(window, Quasar::MouseButton::Left, false);
    window.Update();

    CHECK_FALSE(Quasar::Input::IsKeyPressed(Quasar::Key::A));
    CHECK_FALSE(Quasar::Input::IsMouseButtonPressed(Quasar::MouseButton::Left));
}
