#include "helpers/FakeFileDialog.h"
#include "helpers/HeadlessImGui.h"
#include "helpers/TemporaryFiles.h"

#include <EditorSettings.h>
#include <Project.h>
#include <ProjectPopups.h>

#include <quasar/io/File.h>
#include <quasar/io/Path.h>

#include <doctest/doctest.h>

#include <filesystem>
#include <optional>
#include <utility>
#include <vector>

using Quasar::Editor::EditorSettings;
using Quasar::Editor::OpenProject;
using Quasar::Editor::ProjectPopups;
using Quasar::Editor::ProjectResult;
using TestHelpers::HeadlessImGui;

namespace
{
    std::filesystem::path WriteSettings(const std::filesystem::path& folder, const std::filesystem::path& lastLocation)
    {
        const std::filesystem::path file = folder / "EditorSettings.yaml";
        REQUIRE(Quasar::WriteTextFile(file, "LastLocation: '" + Quasar::PathToUtf8(lastLocation) + "'\n"));
        return file;
    }

    class PopupsFixture
    {
    public:
        explicit PopupsFixture(const std::filesystem::path& location)
            : m_Settings(WriteSettings(m_SettingsFolder.GetPath(), location)), m_Popups(m_Settings, TestHelpers::CreateFakeFileDialogBackend(m_Dialog)),
              m_ImGui(
                  [this]
                  {
                      if (std::optional<ProjectResult> project = m_Popups.Draw())
                      {
                          m_Project = std::move(project);
                      }
                  })
        {
        }

        std::optional<ProjectResult> Frame()
        {
            return Run([this] { m_ImGui.Frame(); });
        }
        std::optional<ProjectResult> Type(const char* text)
        {
            return Run([this, text] { m_ImGui.Type(text); });
        }
        std::optional<ProjectResult> Press(ImGuiKeyChord key)
        {
            return Run([this, key] { m_ImGui.Press(key); });
        }

        void OpenNewProject()
        {
            m_Popups.RequestNewProject();
            Frame();
            Frame();
        }

        std::optional<ProjectResult> ChooseInDialog(const std::optional<std::filesystem::path>& folder)
        {
            REQUIRE(m_Dialog.Callback);
            m_Dialog.Callback({.Folder = folder, .Error = {}});
            return Frame();
        }

        ProjectPopups& GetPopups() { return m_Popups; }
        const EditorSettings& GetSettings() const { return m_Settings; }
        const TestHelpers::FolderDialogRequest& GetDialog() const { return m_Dialog; }

    private:
        template <typename Action>
        std::optional<ProjectResult> Run(Action action)
        {
            m_Project.reset();
            action();
            return std::exchange(m_Project, std::nullopt);
        }

        TestHelpers::TemporaryFolder m_SettingsFolder{"quasar_popups_settings"};
        TestHelpers::FolderDialogRequest m_Dialog;
        EditorSettings m_Settings;
        ProjectPopups m_Popups;
        std::optional<ProjectResult> m_Project;
        HeadlessImGui m_ImGui;
    };

    void MakeProject(const std::filesystem::path& location, const char* name)
    {
        REQUIRE(Quasar::Editor::CreateProject(location, name).IsValid());
    }
}

TEST_SUITE("ProjectPopups")
{
    TEST_CASE("No popup is open before a project is requested")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_closed");
        PopupsFixture imgui(folder.GetPath());

        CHECK_FALSE(imgui.Frame().has_value());
        CHECK_FALSE(HeadlessImGui::IsPopupOpen());
    }

    TEST_CASE("Pressing Enter in the New Project popup creates the project in the given location")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_create");
        PopupsFixture imgui(folder.GetPath());

        imgui.OpenNewProject();
        REQUIRE(HeadlessImGui::IsPopupOpen());
        imgui.Type("MyGame");
        const std::optional<ProjectResult> project = imgui.Press(ImGuiKey_Enter);

        REQUIRE(project.has_value());
        CHECK(project->Directory == (folder.GetPath() / "MyGame").lexically_normal());
        CHECK(project->Name == "MyGame");
        CHECK(OpenProject(project->Directory).IsValid());
        CHECK(imgui.GetSettings().GetRecentProjects() == std::vector{project->Directory});
        CHECK(imgui.GetSettings().GetLastLocation() == folder.GetPath().lexically_normal());
        CHECK_FALSE(HeadlessImGui::IsPopupOpen());
    }

    TEST_CASE("The New Project popup creates nothing without a name")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_no_name");
        PopupsFixture imgui(folder.GetPath());

        imgui.OpenNewProject();

        CHECK_FALSE(imgui.Press(ImGuiKey_Enter).has_value());
        CHECK(HeadlessImGui::IsPopupOpen());
        CHECK(std::filesystem::is_empty(folder.GetPath()));
    }

    TEST_CASE("A project that cannot be created keeps the New Project popup open")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_existing");
        REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "MyGame" / "notes.txt", "content"));
        PopupsFixture imgui(folder.GetPath());

        imgui.OpenNewProject();
        imgui.Type("MyGame");

        CHECK_FALSE(imgui.Press(ImGuiKey_Enter).has_value());
        CHECK(HeadlessImGui::IsPopupOpen());
        CHECK(std::filesystem::exists(folder.GetPath() / "MyGame" / "notes.txt"));
        CHECK_FALSE(std::filesystem::exists(folder.GetPath() / "MyGame" / "Assets"));
    }

    TEST_CASE("Escape closes the New Project popup without creating a project")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_escape");
        PopupsFixture imgui(folder.GetPath());

        imgui.OpenNewProject();
        imgui.Type("MyGame");

        CHECK_FALSE(imgui.Press(ImGuiKey_Escape).has_value());
        CHECK_FALSE(HeadlessImGui::IsPopupOpen());
        CHECK(std::filesystem::is_empty(folder.GetPath()));
    }

    TEST_CASE("A new request clears the name typed in the previous New Project popup")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_reopen");
        PopupsFixture imgui(folder.GetPath());

        imgui.OpenNewProject();
        imgui.Type("MyGame");
        imgui.Press(ImGuiKey_Escape);
        imgui.OpenNewProject();

        CHECK_FALSE(imgui.Press(ImGuiKey_Enter).has_value());
        CHECK(std::filesystem::is_empty(folder.GetPath()));
    }

    TEST_CASE("Open Project opens the project chosen in the folder dialog")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_open");
        MakeProject(folder.GetPath(), "Game");
        PopupsFixture imgui(folder.GetPath());

        imgui.GetPopups().RequestOpenProject();
        imgui.GetPopups().RequestOpenProject();
        CHECK(imgui.GetDialog().Count == 1);
        CHECK(imgui.GetDialog().DefaultLocation == folder.GetPath());
        CHECK_FALSE(imgui.Frame().has_value());

        const std::optional<ProjectResult> project = imgui.ChooseInDialog(folder.GetPath() / "Game");

        REQUIRE(project.has_value());
        CHECK(project->Name == "Game");
        CHECK(project->Directory == folder.GetPath() / "Game");
        CHECK(imgui.GetSettings().GetRecentProjects() == std::vector{folder.GetPath() / "Game"});
        CHECK_FALSE(HeadlessImGui::IsPopupOpen());
    }

    TEST_CASE("Open Project starts the folder dialog in the closest existing folder of the last location")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_open_missing");
        PopupsFixture imgui(folder.GetPath() / "Missing" / "Deeper");

        imgui.GetPopups().RequestOpenProject();

        CHECK(imgui.GetDialog().DefaultLocation == folder.GetPath());
    }

    TEST_CASE("Cancelling the folder dialog of Open Project opens nothing")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_open_cancel");
        PopupsFixture imgui(folder.GetPath());

        imgui.GetPopups().RequestOpenProject();

        CHECK_FALSE(imgui.ChooseInDialog(std::nullopt).has_value());
        CHECK_FALSE(imgui.Frame().has_value());
        CHECK_FALSE(HeadlessImGui::IsPopupOpen());
        CHECK(imgui.GetSettings().GetRecentProjects().empty());
    }

    TEST_CASE("A folder that is not a project shows why it cannot be opened until the user closes the message")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_open_error");
        PopupsFixture imgui(folder.GetPath());

        imgui.GetPopups().RequestOpenProject();

        CHECK_FALSE(imgui.ChooseInDialog(folder.GetPath()).has_value());
        CHECK(HeadlessImGui::IsPopupOpen());
        CHECK_FALSE(imgui.Frame().has_value());
        CHECK(HeadlessImGui::IsPopupOpen());

        imgui.Press(ImGuiKey_Enter);
        CHECK_FALSE(HeadlessImGui::IsPopupOpen());
        CHECK(imgui.GetSettings().GetRecentProjects().empty());
    }

    TEST_CASE("A recent project that cannot be opened anymore leaves the recent projects")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_recent");
        MakeProject(folder.GetPath(), "Kept");
        MakeProject(folder.GetPath(), "Removed");
        PopupsFixture imgui(folder.GetPath());
        imgui.GetPopups().RequestOpenProject(folder.GetPath() / "Kept");
        REQUIRE(imgui.Frame().has_value());
        imgui.GetPopups().RequestOpenProject(folder.GetPath() / "Removed");
        REQUIRE(imgui.Frame().has_value());
        REQUIRE(imgui.GetSettings().GetRecentProjects().size() == 2);

        std::filesystem::remove_all(folder.GetPath() / "Removed");
        imgui.GetPopups().RequestOpenProject(folder.GetPath() / "Removed");

        CHECK_FALSE(imgui.Frame().has_value());
        CHECK(HeadlessImGui::IsPopupOpen());
        CHECK(imgui.GetSettings().GetRecentProjects() == std::vector{folder.GetPath() / "Kept"});
    }

    TEST_CASE("The Browse button of the New Project popup puts the chosen folder in the location")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_browse");
        const std::filesystem::path chosen = folder.GetPath() / "Chosen";
        PopupsFixture imgui(folder.GetPath() / "Missing");

        imgui.OpenNewProject();
        imgui.Type("MyGame");
        imgui.Press(static_cast<ImGuiKeyChord>(ImGuiMod_Shift) | ImGuiKey_Tab);
        imgui.Press(ImGuiKey_Space);
        REQUIRE(imgui.GetDialog().Count == 1);
        CHECK(imgui.GetDialog().DefaultLocation == folder.GetPath());

        CHECK_FALSE(imgui.ChooseInDialog(chosen).has_value());
        imgui.Press(ImGuiKey_Tab);
        const std::optional<ProjectResult> project = imgui.Press(ImGuiKey_Enter);

        REQUIRE(project.has_value());
        CHECK(project->Directory == chosen / "MyGame");
        CHECK(std::filesystem::is_directory(chosen / "MyGame" / "Assets"));
    }
}
