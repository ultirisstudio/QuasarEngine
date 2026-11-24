#include "helpers/LogHelpers.h"
#include "helpers/NullWindow.h"
#include "helpers/TemporaryFiles.h"
#include "helpers/TestLayers.h"

#include <EditorContext.h>
#include <EditorLayer.h>
#include <EditorPanel.h>
#include <EditorSettings.h>

#include <quasar/core/Application.h>
#include <quasar/core/Base.h>
#include <quasar/events/Event.h>
#include <quasar/input/KeyCodes.h>
#include <quasar/log/MemoryLogSink.h>
#include <quasar/time/Timestep.h>

#include <doctest/doctest.h>

#include <imgui.h>

#include <algorithm>
#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace
{
    class RecordingPanel final : public Quasar::Editor::EditorPanel
    {
    public:
        RecordingPanel(Quasar::Editor::EditorContext& context, std::string name, std::vector<std::string>& calls)
            : EditorPanel(std::move(name), context), m_Calls(&calls)
        {
        }

        RecordingPanel(const RecordingPanel&) = delete;
        RecordingPanel& operator=(const RecordingPanel&) = delete;
        RecordingPanel(RecordingPanel&&) = delete;
        RecordingPanel& operator=(RecordingPanel&&) = delete;

        ~RecordingPanel() override { Record("destroyed in " + GetContext().ProjectDirectory.generic_string()); }

        void OnUpdate(Quasar::Timestep /*timestep*/) override { Record("update"); }

        void OnRender() override { Record("render"); }

        void OnEvent(Quasar::Event& event) override
        {
            Record(std::string("event ") + event.GetName());
            event.SetHandled(m_HandlesEvents);
        }

        void SetProjectDirectory(const std::filesystem::path& directory) { GetContext().ProjectDirectory = directory; }

        void SetHandlesEvents(bool handles) { m_HandlesEvents = handles; }

        void SetImGuiAction(std::function<void()> action) { m_ImGuiAction = std::move(action); }

    protected:
        void OnImGuiRender() override
        {
            Record("draw");
            if (m_ImGuiAction)
            {
                m_ImGuiAction();
            }
        }

    private:
        void Record(const std::string& call) { m_Calls->push_back(GetName() + " " + call); }

        std::vector<std::string>* m_Calls;
        bool m_HandlesEvents = false;
        std::function<void()> m_ImGuiAction;
    };

    Quasar::Scope<Quasar::Application> CreateEditorApplication()
    {
        Quasar::Scope<Quasar::Application> application = TestHelpers::CreateApplicationWithContext();
        if (application && ImGui::GetCurrentContext() != nullptr)
        {
            ImGui::GetIO().IniFilename = nullptr;
        }
        return application;
    }

    Quasar::Editor::EditorLayer& AddEditorLayer(Quasar::Application& application)
    {
        auto layer = Quasar::CreateScope<Quasar::Editor::EditorLayer>();
        Quasar::Editor::EditorLayer& editor = *layer;
        application.PushLayer(std::move(layer));
        return editor;
    }

    void RunFrames(Quasar::Application& application, int frames)
    {
        application.PushLayer(Quasar::CreateScope<TestHelpers::ClosingLayer>(frames));
        application.Run();
    }

    std::ptrdiff_t Count(const std::vector<std::string>& calls, const std::string& call)
    {
        return std::ranges::count(calls, call);
    }
}

TEST_CASE("The editor layer updates, renders and draws its panels in the order they were added")
{
    const TestHelpers::ScopedLog log;
    std::vector<std::string> calls;
    const Quasar::Scope<Quasar::Application> application = CreateEditorApplication();
    REQUIRE(application != nullptr);
    Quasar::Editor::EditorLayer& editor = AddEditorLayer(*application);
    editor.AddPanel<RecordingPanel>("A", calls);
    editor.AddPanel<RecordingPanel>("B", calls);

    RunFrames(*application, 1);

    const std::vector<std::string> expected{"A update", "B update", "A render", "B render", "A draw", "B draw"};
    CHECK(calls == expected);
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}

TEST_CASE("A closed panel is still updated, but it is not drawn and has neither the focus nor the cursor")
{
    const TestHelpers::ScopedLog log;
    std::vector<std::string> calls;
    const Quasar::Scope<Quasar::Application> application = CreateEditorApplication();
    REQUIRE(application != nullptr);
    Quasar::Editor::EditorLayer& editor = AddEditorLayer(*application);
    auto& closed = editor.AddPanel<RecordingPanel>("Closed", calls);
    auto& open = editor.AddPanel<RecordingPanel>("Open", calls);
    closed.SetOpen(false);
    open.SetImGuiAction([] { ImGui::SetWindowFocus("Closed"); });

    RunFrames(*application, 2);

    CHECK(Count(calls, "Closed update") == 2);
    CHECK(Count(calls, "Closed draw") == 0);
    CHECK(Count(calls, "Open draw") == 2);
    CHECK_FALSE(closed.IsOpen());
    CHECK_FALSE(closed.IsFocused());
    CHECK_FALSE(closed.IsHovered());
}

TEST_CASE("A panel knows when its window has the focus")
{
    const TestHelpers::ScopedLog log;
    std::vector<std::string> calls;
    const Quasar::Scope<Quasar::Application> application = CreateEditorApplication();
    REQUIRE(application != nullptr);
    Quasar::Editor::EditorLayer& editor = AddEditorLayer(*application);
    const auto& focused = editor.AddPanel<RecordingPanel>("Focused", calls);
    auto& other = editor.AddPanel<RecordingPanel>("Other", calls);
    other.SetImGuiAction([] { ImGui::SetWindowFocus("Focused"); });

    RunFrames(*application, 2);

    CHECK(focused.IsFocused());
    CHECK_FALSE(other.IsFocused());
}

TEST_CASE("Events reach the panels in their order until one of them handles them")
{
    const TestHelpers::ScopedLog log;
    std::vector<std::string> calls;
    const Quasar::Scope<Quasar::Application> application = CreateEditorApplication();
    REQUIRE(application != nullptr);
    Quasar::Editor::EditorLayer& editor = AddEditorLayer(*application);
    editor.AddPanel<RecordingPanel>("A", calls);
    auto& handling = editor.AddPanel<RecordingPanel>("B", calls);
    editor.AddPanel<RecordingPanel>("C", calls);
    handling.SetHandlesEvents(true);

    TestHelpers::SendKey(application->GetWindow(), Quasar::Key::A, true);
    RunFrames(*application, 1);

    CHECK(Count(calls, "A event KeyPressed") == 1);
    CHECK(Count(calls, "B event KeyPressed") == 1);
    CHECK(Count(calls, "C event KeyPressed") == 0);
}

TEST_CASE("Removing the editor layer destroys its panels while the context they share still exists")
{
    const TestHelpers::ScopedLog log;
    std::vector<std::string> calls;
    const Quasar::Scope<Quasar::Application> application = CreateEditorApplication();
    REQUIRE(application != nullptr);
    Quasar::Editor::EditorLayer& editor = AddEditorLayer(*application);
    auto& first = editor.AddPanel<RecordingPanel>("A", calls);
    editor.AddPanel<RecordingPanel>("B", calls);
    first.SetProjectDirectory("Game");

    application->PopLayer(editor);

    const std::vector<std::string> expected{"A destroyed in Game", "B destroyed in Game"};
    CHECK(calls == expected);
}

TEST_CASE("The editor layer reads the recent projects from its settings file and opens no project at start")
{
    const TestHelpers::ScopedLog log;
    const TestHelpers::TemporaryFolder folder("quasar_editor_settings");
    const std::filesystem::path file = folder.GetPath() / "EditorSettings.yaml";
    Quasar::Editor::EditorSettings(file).RememberProject(folder.GetPath() / "Game");
    const Quasar::Scope<Quasar::Application> application = CreateEditorApplication();
    REQUIRE(application != nullptr);
    auto layer = Quasar::CreateScope<Quasar::Editor::EditorLayer>(file);
    const Quasar::Editor::EditorLayer& editor = *layer;
    application->PushLayer(std::move(layer));

    RunFrames(*application, 2);

    CHECK(editor.GetSettings().GetRecentProjects() == std::vector{folder.GetPath() / "Game"});
    CHECK(editor.GetContext().ProjectDirectory.empty());
    CHECK_FALSE(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Error));
}
