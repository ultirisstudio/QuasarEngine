#include "NullWindow.h"

#include <quasar/core/Application.h>
#include <quasar/core/Input.h>
#include <quasar/core/KeyCodes.h>
#include <quasar/core/Layer.h>
#include <quasar/core/MouseCodes.h>
#include <quasar/events/Event.h>

#include <doctest/doctest.h>

#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace
{
    class ScriptLayer : public Quasar::Layer
    {
    public:
        ScriptLayer(std::string name, std::vector<std::string>& calls) : Layer(std::move(name)), Calls(&calls) {}

        void OnAttach() override
        {
            Record("attach");
            if (AttachAction)
            {
                AttachAction();
            }
        }

        void OnDetach() override { Record("detach"); }

        void OnUpdate(Quasar::Timestep /*timestep*/) override
        {
            ++Updates;
            Record("update");
            if (UpdateAction)
            {
                UpdateAction(Updates);
            }
            if (Updates == CloseAfter)
            {
                Quasar::Application::Get().Close();
            }
        }

        void OnRender() override { Record("render"); }

        void OnEvent(Quasar::Event& event) override
        {
            Record(std::string("event ") + event.GetName());
            if (EventAction)
            {
                EventAction(event);
            }
        }

        std::vector<std::string>* Calls;
        int Updates = 0;
        int CloseAfter = 0;
        std::function<void()> AttachAction;
        std::function<void(int)> UpdateAction;
        std::function<void(Quasar::Event&)> EventAction;

    private:
        void Record(const std::string& what) { Calls->push_back(GetName() + " " + what); }
    };

    ScriptLayer& AddLayer(Quasar::Application& application, std::string name, std::vector<std::string>& calls)
    {
        Quasar::Scope<ScriptLayer> layer = Quasar::CreateScope<ScriptLayer>(std::move(name), calls);
        ScriptLayer& added = *layer;
        application.PushLayer(std::move(layer));
        return added;
    }

    ScriptLayer& AddOverlay(Quasar::Application& application, std::string name, std::vector<std::string>& calls)
    {
        Quasar::Scope<ScriptLayer> overlay = Quasar::CreateScope<ScriptLayer>(std::move(name), calls);
        ScriptLayer& added = *overlay;
        application.PushOverlay(std::move(overlay));
        return added;
    }

    std::ptrdiff_t Count(const std::vector<std::string>& calls, const std::string& call)
    {
        return std::ranges::count(calls, call);
    }
}

TEST_CASE("The application runs its layers until it is closed")
{
    const TestHelpers::ScopedLog log;
    std::vector<std::string> calls;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);
    AddLayer(*application, "A", calls).CloseAfter = 3;

    application->Run();

    CHECK(Count(calls, "A update") == 3);
    CHECK(Count(calls, "A render") == 3);
}

TEST_CASE("Layers are updated and rendered from the bottom of the stack to the top")
{
    const TestHelpers::ScopedLog log;
    std::vector<std::string> calls;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);
    AddOverlay(*application, "Overlay", calls).CloseAfter = 1;
    AddLayer(*application, "First", calls);
    AddLayer(*application, "Second", calls);
    calls.clear();

    application->Run();

    const std::vector<std::string> expected{"First update", "Second update", "Overlay update",
                                            "First render", "Second render", "Overlay render"};
    CHECK(calls == expected);
}

TEST_CASE("Events reach the top layer first and stop once handled")
{
    const TestHelpers::ScopedLog log;
    std::vector<std::string> calls;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);
    ScriptLayer& bottom = AddLayer(*application, "Bottom", calls);
    ScriptLayer& overlay = AddOverlay(*application, "Overlay", calls);
    overlay.EventAction = [](Quasar::Event& event) { event.SetHandled(true); };
    bottom.UpdateAction = [&application](int /*updates*/)
    { TestHelpers::SendKey(application->GetWindow(), GLFW_KEY_A, GLFW_PRESS); };
    bottom.CloseAfter = 1;

    application->Run();

    CHECK(Count(calls, "Overlay event KeyPressed") == 1);
    CHECK(Count(calls, "Bottom event KeyPressed") == 0);
}

TEST_CASE("A close request of the window stops the main loop")
{
    const TestHelpers::ScopedLog log;
    std::vector<std::string> calls;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);
    ScriptLayer& layer = AddLayer(*application, "A", calls);
    layer.UpdateAction = [&application](int updates)
    {
        if (updates == 2)
        {
            TestHelpers::SendCloseRequest(application->GetWindow());
        }
    };

    application->Run();

    CHECK(layer.Updates == 2);
    CHECK(Count(calls, "A event WindowClose") == 0);
}

TEST_CASE("While the window is minimized the layers are updated but not rendered")
{
    const TestHelpers::ScopedLog log;
    std::vector<std::string> calls;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);
    ScriptLayer& layer = AddLayer(*application, "A", calls);
    layer.UpdateAction = [&application](int updates)
    {
        if (updates == 1)
        {
            glfwIconifyWindow(application->GetWindow().GetNativeWindow());
        }
        if (updates == 3)
        {
            glfwRestoreWindow(application->GetWindow().GetNativeWindow());
        }
    };
    layer.CloseAfter = 5;

    application->Run();

    CHECK(Count(calls, "A update") == 5);
    CHECK(Count(calls, "A render") == 3);
}

TEST_CASE("Only one application can exist at a time")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication({.Name = "First"});
    REQUIRE(application != nullptr);

    const Quasar::Scope<Quasar::Application> second = TestHelpers::CreateNullApplication({.Name = "Second"});

    CHECK(second == nullptr);
    CHECK(&Quasar::Application::Get() == application.get());
}

TEST_CASE("The application keeps its specification")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application =
        TestHelpers::CreateNullApplication({.Name = "Game", .Arguments = {"--level", "2"}});
    REQUIRE(application != nullptr);

    CHECK(application->GetSpecification().Name == "Game");
    CHECK(application->GetSpecification().Arguments == std::vector<std::string>{"--level", "2"});
}

TEST_CASE("Layers added before the main loop are attached right away")
{
    const TestHelpers::ScopedLog log;
    std::vector<std::string> calls;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);

    ScriptLayer& layer = AddLayer(*application, "A", calls);
    CHECK(calls == std::vector<std::string>{"A attach"});

    application->PopLayer(layer);
    CHECK(calls == std::vector<std::string>{"A attach", "A detach"});
}

TEST_CASE("Layers added during a frame start at the next frame")
{
    const TestHelpers::ScopedLog log;
    std::vector<std::string> calls;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);
    ScriptLayer& layer = AddLayer(*application, "A", calls);
    layer.UpdateAction = [&application, &calls](int updates)
    {
        if (updates == 1)
        {
            AddLayer(*application, "B", calls);
            AddOverlay(*application, "Overlay", calls);
        }
    };
    layer.CloseAfter = 2;
    calls.clear();

    application->Run();

    const std::vector<std::string> expected{
        "A update", "A render",       "B attach", "Overlay attach", "A update",
        "B update", "Overlay update", "A render", "B render",       "Overlay render",
    };
    CHECK(calls == expected);
}

TEST_CASE("A layer can remove itself during an update")
{
    const TestHelpers::ScopedLog log;
    std::vector<std::string> calls;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);
    ScriptLayer& removed = AddLayer(*application, "Removed", calls);
    AddOverlay(*application, "Overlay", calls).CloseAfter = 2;
    removed.UpdateAction = [&application, &removed](int /*updates*/) { application->PopLayer(removed); };
    calls.clear();

    application->Run();

    const std::vector<std::string> expected{
        "Removed update", "Overlay update", "Removed render", "Overlay render",
        "Removed detach", "Overlay update", "Overlay render",
    };
    CHECK(calls == expected);
}

TEST_CASE("A layer can remove itself when it receives an event")
{
    const TestHelpers::ScopedLog log;
    std::vector<std::string> calls;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);
    ScriptLayer& removed = AddLayer(*application, "Removed", calls);
    ScriptLayer& overlay = AddOverlay(*application, "Overlay", calls);
    removed.EventAction = [&application, &removed](Quasar::Event& /*event*/) { application->PopLayer(removed); };
    overlay.UpdateAction = [&application](int updates)
    {
        if (updates == 1)
        {
            TestHelpers::SendKey(application->GetWindow(), GLFW_KEY_A, GLFW_PRESS);
        }
    };
    overlay.CloseAfter = 2;

    application->Run();

    CHECK(Count(calls, "Removed event KeyPressed") == 1);
    CHECK(Count(calls, "Removed detach") == 1);
    CHECK(Count(calls, "Removed update") == 1);
    CHECK(Count(calls, "Overlay update") == 2);
}

TEST_CASE("Layers added while the changes are applied are added in the same frame")
{
    const TestHelpers::ScopedLog log;
    std::vector<std::string> calls;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);
    ScriptLayer& layer = AddLayer(*application, "A", calls);
    layer.UpdateAction = [&application, &calls](int updates)
    {
        if (updates == 1)
        {
            ScriptLayer& parent = AddLayer(*application, "Parent", calls);
            parent.AttachAction = [&application, &calls] { AddLayer(*application, "Child", calls); };
        }
    };
    layer.CloseAfter = 2;
    calls.clear();

    application->Run();

    const std::vector<std::string> expected{
        "A update",      "A render",     "Parent attach", "Child attach",  "A update",
        "Parent update", "Child update", "A render",      "Parent render", "Child render",
    };
    CHECK(calls == expected);
}

TEST_CASE("Input reads the state of the main window")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);

    glfwSetCursorPos(application->GetWindow().GetNativeWindow(), 12.0, 34.0);

    CHECK(Quasar::Input::GetMousePosition() == glm::vec2(12.0f, 34.0f));
    CHECK_FALSE(Quasar::Input::IsKeyPressed(Quasar::Key::A));
    CHECK_FALSE(Quasar::Input::IsMouseButtonPressed(Quasar::MouseButton::Left));
}
