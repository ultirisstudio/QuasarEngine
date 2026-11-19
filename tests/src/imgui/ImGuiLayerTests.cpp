#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"

#include <quasar/core/Application.h>
#include <quasar/core/Layer.h>
#include <quasar/events/Event.h>
#include <quasar/imgui/ImGuiLayer.h>
#include <quasar/input/Input.h>
#include <quasar/input/KeyCodes.h>
#include <quasar/input/MouseCodes.h>
#include <quasar/log/MemoryLogSink.h>

#include <doctest/doctest.h>

#include <glm/vec2.hpp>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace
{
    class ImGuiTestLayer : public Quasar::Layer
    {
    public:
        explicit ImGuiTestLayer(std::string name) : Layer(std::move(name)) {}

        void OnUpdate(Quasar::Timestep /*timestep*/) override
        {
            ++Updates;
            if (UpdateAction)
            {
                UpdateAction(Updates);
            }
            if (Updates == CloseAfter)
            {
                Quasar::Application::Get().Close();
            }
        }

        void OnImGuiRender() override
        {
            ++ImGuiRenders;
            if (ImGuiAction)
            {
                ImGuiAction();
            }
        }

        void OnEvent(Quasar::Event& event) override { Events.push_back(event.GetType()); }

        int Updates = 0;
        int ImGuiRenders = 0;
        int CloseAfter = 3;
        std::vector<Quasar::EventType> Events;
        std::function<void(int)> UpdateAction;
        std::function<void()> ImGuiAction;
    };

    Quasar::Scope<Quasar::Application> CreateImGuiApplication(const Quasar::ImGuiSpecification& imgui = {})
    {
        Quasar::Scope<Quasar::Application> application = TestHelpers::CreateApplicationWithContext({.ImGui = imgui});
        if (application != nullptr && ImGui::GetCurrentContext() != nullptr)
        {
            ImGui::GetIO().IniFilename = nullptr;
        }
        return application;
    }

    ImGuiTestLayer& AddLayer(Quasar::Application& application, std::string name)
    {
        Quasar::Scope<ImGuiTestLayer> layer = Quasar::CreateScope<ImGuiTestLayer>(std::move(name));
        ImGuiTestLayer& added = *layer;
        application.PushLayer(std::move(layer));
        return added;
    }

    void DrawCornerWindow()
    {
        ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
        ImGui::SetNextWindowSize(ImVec2(300.0f, 300.0f));
        ImGui::Begin("Corner");
        ImGui::TextUnformatted("Over the corner");
        ImGui::End();
    }

    void ClickOverImGui(Quasar::Application& application, int updates)
    {
        if (updates == 1)
        {
            TestHelpers::SendMouseMotion(application.GetWindow(), 100.0f, 100.0f);
        }
        if (updates == 3)
        {
            TestHelpers::SendMouseButton(application.GetWindow(), Quasar::MouseButton::Left, true);
            TestHelpers::SendMouseButton(application.GetWindow(), Quasar::MouseButton::Left, false);
        }
    }

    void TypeLetter(Quasar::Application& application, int updates)
    {
        if (updates == 3)
        {
            TestHelpers::SendKey(application.GetWindow(), Quasar::Key::A, true);
            TestHelpers::SendText(application.GetWindow(), "a");
            TestHelpers::SendKey(application.GetWindow(), Quasar::Key::A, false);
        }
    }

    struct HeldInput
    {
        bool Read = false;
        bool KeyPressed = false;
        bool KeyDown = false;
        bool ButtonPressed = false;
        bool ButtonDown = false;
        glm::vec2 Delta{0.0f};
        glm::vec2 Position{0.0f};
    };

    void HoldOverImGui(Quasar::Application& application, int updates, HeldInput& held)
    {
        const Quasar::Window& window = application.GetWindow();
        if (updates == 1)
        {
            TestHelpers::SendMouseMotion(window, 100.0f, 100.0f);
        }
        if (updates == 3)
        {
            TestHelpers::SendKey(window, Quasar::Key::W, true);
            TestHelpers::SendMouseButton(window, Quasar::MouseButton::Left, true);
            TestHelpers::SendMouseMotion(window, 110.0f, 105.0f, 10.0f, 5.0f);
        }
        if (updates == 4)
        {
            held = {.Read = true,
                    .KeyPressed = Quasar::Input::IsKeyPressed(Quasar::Key::W),
                    .KeyDown = window.IsKeyDown(Quasar::Key::W),
                    .ButtonPressed = Quasar::Input::IsMouseButtonPressed(Quasar::MouseButton::Left),
                    .ButtonDown = window.IsMouseButtonDown(Quasar::MouseButton::Left),
                    .Delta = Quasar::Input::GetMouseDelta(),
                    .Position = Quasar::Input::GetMousePosition()};
        }
    }

    void DrawFocusedTextField(std::array<char, 64>& text)
    {
        DrawCornerWindow();
        ImGui::Begin("Text");
        ImGui::SetKeyboardFocusHere();
        ImGui::InputText("Name", text.data(), text.size());
        ImGui::End();
    }

    HeldInput RunHoldingOverImGui(Quasar::Application& application, ImGuiTestLayer& layer, const std::function<void()>& draw)
    {
        HeldInput held;
        layer.CloseAfter = 4;
        layer.ImGuiAction = draw;
        layer.UpdateAction = [&application, &held](int updates) { HoldOverImGui(application, updates, held); };
        application.Run();
        REQUIRE(held.Read);
        return held;
    }

    bool Received(const ImGuiTestLayer& layer, Quasar::EventType type)
    {
        return std::ranges::find(layer.Events, type) != layer.Events.end();
    }
}

TEST_CASE("An application without a graphics context has no ImGui layer")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);

    CHECK(application->GetImGuiLayer() == nullptr);
    CHECK(ImGui::GetCurrentContext() == nullptr);
}

TEST_CASE("ImGui starts with the application and stops with it")
{
    const TestHelpers::ScopedLog log;
    {
        const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication();
        REQUIRE(application != nullptr);

        CHECK(application->GetImGuiLayer() != nullptr);
        CHECK(ImGui::GetCurrentContext() != nullptr);
    }

    CHECK(ImGui::GetCurrentContext() == nullptr);
}

TEST_CASE("Every layer can draw ImGui windows from its ImGui hook")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication();
    REQUIRE(application != nullptr);
    ImGuiTestLayer& first = AddLayer(*application, "First");
    ImGuiTestLayer& second = AddLayer(*application, "Second");
    std::vector<int> frames;
    first.ImGuiAction = [&frames]
    {
        frames.push_back(ImGui::GetFrameCount());
        ImGui::Begin("First window");
        ImGui::TextUnformatted("Drawn by the first layer");
        ImGui::End();
    };
    second.ImGuiAction = []
    {
        ImGui::Begin("Second window");
        ImGui::Button("Drawn by the second layer");
        ImGui::End();
    };

    application->Run();

    CHECK(first.ImGuiRenders == 3);
    CHECK(second.ImGuiRenders == 3);
    CHECK(frames == std::vector<int>{1, 2, 3});
}

TEST_CASE("The ImGui demo window can be displayed")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication();
    REQUIRE(application != nullptr);
    ImGuiTestLayer& layer = AddLayer(*application, "Demo");
    layer.ImGuiAction = [] { ImGui::ShowDemoWindow(); };

    application->Run();

    CHECK(layer.ImGuiRenders == 3);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("ImGui keeps the mouse from the layers while the cursor is over an ImGui window")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication();
    REQUIRE(application != nullptr);
    ImGuiTestLayer& layer = AddLayer(*application, "Scene");
    layer.ImGuiAction = DrawCornerWindow;
    layer.UpdateAction = [&application](int updates) { ClickOverImGui(*application, updates); };

    application->Run();

    CHECK_FALSE(Received(layer, Quasar::EventType::MouseButtonPressed));
    CHECK_FALSE(Received(layer, Quasar::EventType::MouseButtonReleased));
}

TEST_CASE("The layers receive the mouse when ImGui does not block events")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication();
    REQUIRE(application != nullptr);
    REQUIRE(application->GetImGuiLayer() != nullptr);
    application->GetImGuiLayer()->SetBlockEvents(false);
    ImGuiTestLayer& layer = AddLayer(*application, "Scene");
    layer.ImGuiAction = DrawCornerWindow;
    layer.UpdateAction = [&application](int updates) { ClickOverImGui(*application, updates); };

    application->Run();

    CHECK(Received(layer, Quasar::EventType::MouseButtonPressed));
    CHECK(Received(layer, Quasar::EventType::MouseButtonReleased));
}

TEST_CASE("ImGui windows can be docked")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication();
    REQUIRE(application != nullptr);
    ImGuiTestLayer& layer = AddLayer(*application, "Docking");
    bool docked = false;
    layer.ImGuiAction = [&docked]
    {
        const ImGuiID dockSpace = ImGui::DockSpaceOverViewport();
        ImGui::SetNextWindowDockID(dockSpace, ImGuiCond_Always);
        ImGui::Begin("Docked window");
        docked = ImGui::IsWindowDocked();
        ImGui::End();
    };

    application->Run();

    CHECK(docked);
}

TEST_CASE("ImGui windows can be dragged outside the main window")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication();
    REQUIRE(application != nullptr);
    REQUIRE((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0);
    ImGuiTestLayer& layer = AddLayer(*application, "Viewports");
    bool outside = false;
    int viewports = 0;
    layer.ImGuiAction = [&outside, &viewports]
    {
        const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(mainViewport->Pos.x + mainViewport->Size.x + 20.0f, mainViewport->Pos.y + 20.0f));
        ImGui::SetNextWindowSize(ImVec2(200.0f, 200.0f));
        ImGui::Begin("Outside");
        outside = ImGui::GetWindowViewport() != mainViewport;
        ImGui::End();
        viewports = ImGui::GetPlatformIO().Viewports.Size;
    };

    application->Run();

    CHECK(outside);
    CHECK(viewports == 2);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("An application can keep its ImGui windows undocked and inside the main window")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication({.Docking = false, .Viewports = false});
    REQUIRE(application != nullptr);
    ImGuiTestLayer& layer = AddLayer(*application, "Game");
    bool inside = false;
    int viewports = 0;
    layer.ImGuiAction = [&inside, &viewports]
    {
        const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(mainViewport->Pos.x + mainViewport->Size.x + 20.0f, mainViewport->Pos.y + 20.0f));
        ImGui::Begin("Outside");
        inside = ImGui::GetWindowViewport() == mainViewport;
        ImGui::End();
        viewports = ImGui::GetPlatformIO().Viewports.Size;
    };

    CHECK((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable) == 0);
    CHECK((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) == 0);
    application->Run();

    CHECK(inside);
    CHECK(viewports == 1);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("ImGui keeps the keyboard from the layers while a text field has the focus")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication();
    REQUIRE(application != nullptr);
    ImGuiTestLayer& layer = AddLayer(*application, "Scene");
    std::array<char, 64> text{};
    layer.ImGuiAction = [&text]
    {
        ImGui::Begin("Text");
        ImGui::SetKeyboardFocusHere();
        ImGui::InputText("Name", text.data(), text.size());
        ImGui::End();
    };
    layer.UpdateAction = [&application](int updates) { TypeLetter(*application, updates); };

    application->Run();

    CHECK_FALSE(Received(layer, Quasar::EventType::KeyPressed));
    CHECK_FALSE(Received(layer, Quasar::EventType::KeyTyped));
    CHECK_FALSE(Received(layer, Quasar::EventType::KeyReleased));
}

TEST_CASE("The layers receive the keyboard while ImGui does not use it")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication();
    REQUIRE(application != nullptr);
    ImGuiTestLayer& layer = AddLayer(*application, "Scene");
    layer.UpdateAction = [&application](int updates) { TypeLetter(*application, updates); };

    application->Run();

    CHECK(Received(layer, Quasar::EventType::KeyPressed));
    CHECK(Received(layer, Quasar::EventType::KeyTyped));
    CHECK(Received(layer, Quasar::EventType::KeyReleased));
}

TEST_CASE("ImGui ignores the mouse and the keyboard while the cursor is captured, then gives back only the flags it changed")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication();
    REQUIRE(application != nullptr);
    ImGuiTestLayer& layer = AddLayer(*application, "Scene");
    layer.ImGuiAction = DrawCornerWindow;
    layer.UpdateAction = [&application](int updates) { ClickOverImGui(*application, updates); };
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    REQUIRE(application->GetWindow().SetCursorCaptured(true));

    application->Run();

    CHECK(Received(layer, Quasar::EventType::MouseButtonPressed));
    CHECK((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NoMouse) != 0);
    CHECK((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NoKeyboard) != 0);

    REQUIRE(application->GetWindow().SetCursorCaptured(false));
    layer.Events.clear();
    layer.CloseAfter = 6;
    layer.UpdateAction = [&application](int updates) { ClickOverImGui(*application, updates - 3); };
    application->Run();

    CHECK_FALSE(Received(layer, Quasar::EventType::MouseButtonPressed));
    CHECK((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NoMouse) == 0);
    CHECK((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NoKeyboard) == 0);
    CHECK((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) != 0);
}

TEST_CASE("An application without ImGui never calls the ImGui hook, and nothing blocks the input of its layers")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication({.Enabled = false});
    REQUIRE(application != nullptr);
    CHECK(application->GetImGuiLayer() == nullptr);
    CHECK(ImGui::GetCurrentContext() == nullptr);

    ImGuiTestLayer& layer = AddLayer(*application, "Scene");
    const HeldInput held = RunHoldingOverImGui(*application, layer, {});

    CHECK(held.KeyPressed);
    CHECK(held.ButtonPressed);
    CHECK(held.Delta == glm::vec2(10.0f, 5.0f));
    CHECK(layer.Updates == 4);
    CHECK(layer.ImGuiRenders == 0);
    CHECK(Received(layer, Quasar::EventType::KeyPressed));
    CHECK(Received(layer, Quasar::EventType::MouseButtonPressed));
}

TEST_CASE("Input reads the keyboard and the mouse as released and still while ImGui blocks them, but not the window")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication();
    REQUIRE(application != nullptr);
    std::array<char, 64> text{};

    ImGuiTestLayer& layer = AddLayer(*application, "Scene");
    const HeldInput held = RunHoldingOverImGui(*application, layer, [&text] { DrawFocusedTextField(text); });

    REQUIRE(application->GetImGuiLayer() != nullptr);
    CHECK(application->GetImGuiLayer()->IsBlockingKeyboard());
    CHECK(application->GetImGuiLayer()->IsBlockingMouse());
    CHECK_FALSE(held.KeyPressed);
    CHECK(held.KeyDown);
    CHECK_FALSE(held.ButtonPressed);
    CHECK(held.ButtonDown);
    CHECK(held.Delta == glm::vec2(0.0f));
    CHECK(held.Position == glm::vec2(110.0f, 105.0f));

    REQUIRE(application->GetWindow().SetCursorCaptured(true));
    CHECK_FALSE(application->GetImGuiLayer()->IsBlockingKeyboard());
    CHECK_FALSE(application->GetImGuiLayer()->IsBlockingMouse());
}

TEST_CASE("Input reads the keyboard and the mouse that ImGui uses when the ImGui layer does not block events")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication();
    REQUIRE(application != nullptr);
    application->GetImGuiLayer()->SetBlockEvents(false);
    std::array<char, 64> text{};

    ImGuiTestLayer& layer = AddLayer(*application, "Scene");
    const HeldInput held = RunHoldingOverImGui(*application, layer, [&text] { DrawFocusedTextField(text); });

    CHECK_FALSE(application->GetImGuiLayer()->IsBlockingKeyboard());
    CHECK_FALSE(application->GetImGuiLayer()->IsBlockingMouse());
    CHECK(held.KeyPressed);
    CHECK(held.ButtonPressed);
    CHECK(held.Delta == glm::vec2(10.0f, 5.0f));
}

TEST_CASE("While the cursor is captured, ImGui blocks neither the keyboard nor the mouse")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication();
    REQUIRE(application != nullptr);
    std::array<char, 64> text{};
    REQUIRE(application->GetWindow().SetCursorCaptured(true));

    ImGuiTestLayer& layer = AddLayer(*application, "Scene");
    const HeldInput held = RunHoldingOverImGui(*application, layer, [&text] { DrawFocusedTextField(text); });

    CHECK_FALSE(application->GetImGuiLayer()->IsBlockingKeyboard());
    CHECK_FALSE(application->GetImGuiLayer()->IsBlockingMouse());
    CHECK(held.KeyPressed);
    CHECK(held.ButtonPressed);
}

TEST_CASE("An ImGui layer that is not attached blocks nothing")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Window> window = TestHelpers::CreateWindowWithContext();
    REQUIRE(window != nullptr);
    const Quasar::ImGuiLayer layer(*window);

    CHECK_FALSE(layer.IsBlockingKeyboard());
    CHECK_FALSE(layer.IsBlockingMouse());
}
