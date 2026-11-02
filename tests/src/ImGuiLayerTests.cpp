#include "NullWindow.h"

#include <quasar/core/Application.h>
#include <quasar/core/ImGuiLayer.h>
#include <quasar/core/Layer.h>
#include <quasar/core/MemoryLogSink.h>
#include <quasar/core/PlatformDetection.h>
#include <quasar/events/Event.h>

#include <doctest/doctest.h>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <algorithm>
#include <array>
#include <functional>
#include <string>
#include <utility>
#include <vector>

TEST_CASE("An application without an OpenGL context has no ImGui layer")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplication();
    REQUIRE(application != nullptr);

    CHECK(application->GetImGuiLayer() == nullptr);
    CHECK(ImGui::GetCurrentContext() == nullptr);
}

// ImGui draws with OpenGL: these tests need the Mesa software driver through EGL, only on Linux
#ifdef QUASAR_PLATFORM_LINUX

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

        void OnEvent(Quasar::Event& event) override { Events.emplace_back(event.GetName()); }

        int Updates = 0;
        int ImGuiRenders = 0;
        int CloseAfter = 3;
        std::vector<std::string> Events;
        std::function<void(int)> UpdateAction;
        std::function<void()> ImGuiAction;
    };

    Quasar::Scope<Quasar::Application> CreateImGuiApplication()
    {
        Quasar::Scope<Quasar::Application> application = TestHelpers::CreateNullApplicationWithContext();
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
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(300.0f, 300.0f));
        ImGui::Begin("Corner");
        ImGui::TextUnformatted("Over the corner");
        ImGui::End();
    }

    void ClickOverImGui(Quasar::Application& application, int updates)
    {
        if (updates == 1)
        {
            glfwSetCursorPos(application.GetWindow().GetNativeWindow(), 100.0, 100.0);
            TestHelpers::SendCursorPosition(application.GetWindow(), 100.0, 100.0);
        }
        if (updates == 3)
        {
            TestHelpers::SendMouseButton(application.GetWindow(), GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
            TestHelpers::SendMouseButton(application.GetWindow(), GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE);
        }
    }

    void TypeLetter(Quasar::Application& application, int updates)
    {
        if (updates == 3)
        {
            TestHelpers::SendKey(application.GetWindow(), GLFW_KEY_A, GLFW_PRESS);
            TestHelpers::SendCharacter(application.GetWindow(), 'a');
            TestHelpers::SendKey(application.GetWindow(), GLFW_KEY_A, GLFW_RELEASE);
        }
    }

    bool Received(const ImGuiTestLayer& layer, const std::string& name)
    {
        return std::ranges::find(layer.Events, name) != layer.Events.end();
    }
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
    const std::vector<Quasar::LogMessage> messages = Quasar::Log::GetMemorySink()->GetMessages();
    CHECK(std::ranges::none_of(messages, [](const Quasar::LogMessage& message) { return message.Level >= Quasar::LogLevel::Error; }));
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

    CHECK_FALSE(Received(layer, "MouseButtonPressed"));
    CHECK_FALSE(Received(layer, "MouseButtonReleased"));
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

    CHECK(Received(layer, "MouseButtonPressed"));
    CHECK(Received(layer, "MouseButtonReleased"));
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
    const std::vector<Quasar::LogMessage> messages = Quasar::Log::GetMemorySink()->GetMessages();
    CHECK(std::ranges::none_of(messages, [](const Quasar::LogMessage& message) { return message.Level >= Quasar::LogLevel::Warning; }));
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

    CHECK_FALSE(Received(layer, "KeyPressed"));
    CHECK_FALSE(Received(layer, "KeyTyped"));
    CHECK_FALSE(Received(layer, "KeyReleased"));
}

TEST_CASE("The layers receive the keyboard while ImGui does not use it")
{
    const TestHelpers::ScopedLog log;
    const Quasar::Scope<Quasar::Application> application = CreateImGuiApplication();
    REQUIRE(application != nullptr);
    ImGuiTestLayer& layer = AddLayer(*application, "Scene");
    layer.UpdateAction = [&application](int updates) { TypeLetter(*application, updates); };

    application->Run();

    CHECK(Received(layer, "KeyPressed"));
    CHECK(Received(layer, "KeyTyped"));
    CHECK(Received(layer, "KeyReleased"));
}

#endif
