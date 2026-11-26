#include "helpers/EditorLocale.h"
#include "helpers/HeadlessImGui.h"
#include "helpers/LogHelpers.h"
#include "helpers/TemporaryFiles.h"

#include <EditorContext.h>
#include <Project.h>
#include <panels/ContentBrowserPanel.h>

#include <quasar/io/File.h>
#include <quasar/log/Log.h>
#include <quasar/time/Timestep.h>

#include <doctest/doctest.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

using Quasar::Timestep;
using Quasar::Editor::ContentBrowserEntry;
using Quasar::Editor::ContentBrowserPanel;
using Quasar::Editor::ContentBrowserTreeNode;
using Quasar::Editor::CreateProject;
using Quasar::Editor::EditorContext;
using Quasar::Editor::GetAssetsDirectory;
using Quasar::Editor::ProjectResult;

namespace
{
    constexpr const char* WindowName = "###panel.content_browser";

    constexpr ImVec2 CardSize{96.0f, 112.0f};

    std::filesystem::path CreateTestProject(const TestHelpers::TemporaryFolder& folder, std::string_view name)
    {
        const ProjectResult project = CreateProject(folder.GetPath(), name);
        REQUIRE(project.IsValid());
        return project.Directory;
    }

    std::vector<std::filesystem::path> GetEntryNames(const ContentBrowserPanel& panel)
    {
        std::vector<std::filesystem::path> names;
        for (const ContentBrowserEntry& entry : panel.GetEntries())
        {
            names.push_back(entry.Path.filename());
        }
        return names;
    }

    ImGuiWindow* FindChildWindow(std::string_view name)
    {
        const ImGuiWindow* panel = ImGui::FindWindowByName(WindowName);
        REQUIRE(panel != nullptr);
        const std::string part = "/" + std::string(name) + "_";
        for (ImGuiWindow* window : ImGui::GetCurrentContext()->Windows)
        {
            if (window->ParentWindow == panel && std::string_view(window->Name).find(part) != std::string_view::npos)
            {
                return window;
            }
        }
        FAIL("The child window is not drawn");
        return nullptr;
    }

    void ClickAt(const TestHelpers::HeadlessImGui& imgui, ImVec2 point, int clicks = 1)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.AddMousePosEvent(point.x, point.y);
        imgui.Frame();
        for (int click = 0; click < clicks; ++click)
        {
            io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
            imgui.Frame();
            io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
            imgui.Frame();
        }
    }

    void ClickCard(const TestHelpers::HeadlessImGui& imgui, int index, int clicks = 1)
    {
        const ImVec2 start = FindChildWindow("Entries")->DC.CursorStartPos;
        const float step = CardSize.x + ImGui::GetStyle().ItemSpacing.x;
        ClickAt(imgui, ImVec2{start.x + (step * static_cast<float>(index)) + (CardSize.x / 2.0f), start.y + (CardSize.y / 2.0f)}, clicks);
    }

    void ClickParentFolder(const TestHelpers::HeadlessImGui& imgui)
    {
        const ImGuiWindow* entries = FindChildWindow("Entries");
        const float size =   ImGui::GetFrameHeight();
        ClickAt(imgui, ImVec2{entries->Pos.x + (size / 2.0f), entries->Pos.y - ImGui::GetStyle().ItemSpacing.y - (size / 2.0f)});
    }

    void ClickTreeRow(const TestHelpers::HeadlessImGui& imgui, int row)
    {
        const ImGuiWindow* tree = FindChildWindow("Tree");
        const float rowHeight = ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y;
        const ImVec2 start = tree->DC.CursorStartPos;
        ClickAt(imgui, ImVec2{start.x + (tree->Size.x / 2.0f), start.y + (rowHeight * static_cast<float>(row)) + (ImGui::GetTextLineHeight() / 2.0f)});
    }
}

TEST_CASE("The content browser shows no folder while no project is open")
{
    EditorContext context;
    ContentBrowserPanel panel(context);

    panel.OnUpdate(Timestep{});

    CHECK(panel.GetShownDirectory().empty());
    CHECK(panel.GetEntries().empty());
}

TEST_CASE("The content browser shows the Assets folder of the project that is opened")
{
    const TestHelpers::TemporaryFolder folder;
    const std::filesystem::path project = CreateTestProject(folder, "Game");
    EditorContext context;
    ContentBrowserPanel panel(context);
    panel.OnUpdate(Timestep{});

    context.ProjectDirectory = project;
    panel.OnUpdate(Timestep{});

    CHECK(panel.GetShownDirectory() == GetAssetsDirectory(project));
}

TEST_CASE("The content browser follows the project when another one is opened or when it is closed")
{
    const TestHelpers::TemporaryFolder folder;
    const std::filesystem::path first = CreateTestProject(folder, "First");
    const std::filesystem::path second = CreateTestProject(folder, "Second");
    EditorContext context{first};
    ContentBrowserPanel panel(context);
    panel.OnUpdate(Timestep{});

    context.ProjectDirectory = second;
    panel.OnUpdate(Timestep{});
    CHECK(panel.GetShownDirectory() == GetAssetsDirectory(second));

    context.ProjectDirectory.clear();
    panel.OnUpdate(Timestep{});
    CHECK(panel.GetShownDirectory().empty());
}

TEST_CASE("The content browser lists the folders before the files and sorts each of them by name")
{
    const TestHelpers::TemporaryFolder folder;
    const std::filesystem::path project = CreateTestProject(folder, "Game");
    const std::filesystem::path assets = GetAssetsDirectory(project);
    REQUIRE(Quasar::WriteTextFile(assets / "b.txt", "b"));
    REQUIRE(Quasar::CreateDirectories(assets / "Textures"));
    REQUIRE(Quasar::WriteTextFile(assets / "a.txt", "a"));
    REQUIRE(Quasar::CreateDirectories(assets / "Audio"));
    EditorContext context{project};
    ContentBrowserPanel panel(context);

    panel.OnUpdate(Timestep{});

    CHECK(GetEntryNames(panel) == std::vector<std::filesystem::path>{"Audio", "Textures", "a.txt", "b.txt"});
    REQUIRE(panel.GetEntries().size() == 4);
    CHECK(panel.GetEntries()[0].IsDirectory);
    CHECK(panel.GetEntries()[1].IsDirectory);
    CHECK_FALSE(panel.GetEntries()[2].IsDirectory);
    CHECK_FALSE(panel.GetEntries()[3].IsDirectory);
}

TEST_CASE("The content browser lists nothing in the empty Assets folder of a new project")
{
    const TestHelpers::TemporaryFolder folder;
    EditorContext context{CreateTestProject(folder, "Game")};
    ContentBrowserPanel panel(context);

    panel.OnUpdate(Timestep{});

    CHECK(panel.GetEntries().empty());
}

TEST_CASE("The content browser lists nothing and logs a warning when the Assets folder cannot be read")
{
    const TestHelpers::ScopedLog log;
    const TestHelpers::TemporaryFolder folder;
    EditorContext context{folder.GetPath() / "Missing"};
    ContentBrowserPanel panel(context);

    panel.OnUpdate(Timestep{});

    CHECK(panel.GetEntries().empty());
    CHECK(TestHelpers::HasLogAtLeast(Quasar::LogLevel::Warning));
}

TEST_CASE("Clicking a card in the content browser selects its folder or its file")
{
    TestHelpers::UseEditorLocale();
    const TestHelpers::TemporaryFolder folder;
    const std::filesystem::path project = CreateTestProject(folder, "Game");
    const std::filesystem::path assets = GetAssetsDirectory(project);
    REQUIRE(Quasar::CreateDirectories(assets / "Textures"));
    REQUIRE(Quasar::WriteTextFile(assets / "notes.txt", "notes"));
    EditorContext context{project};
    ContentBrowserPanel panel(context);
    panel.OnUpdate(Timestep{});
    const TestHelpers::HeadlessImGui imgui([&] { panel.Draw(); });
    imgui.Frame();
    CHECK(panel.GetSelected().empty());

    ClickCard(imgui, 0);
    CHECK(panel.GetSelected() == assets / "Textures");

    ClickCard(imgui, 1);
    CHECK(panel.GetSelected() == assets / "notes.txt");
}

TEST_CASE("The selection of the content browser is cleared when another project is opened")
{
    TestHelpers::UseEditorLocale();
    const TestHelpers::TemporaryFolder folder;
    const std::filesystem::path first = CreateTestProject(folder, "First");
    REQUIRE(Quasar::CreateDirectories(GetAssetsDirectory(first) / "Textures"));
    EditorContext context{first};
    ContentBrowserPanel panel(context);
    panel.OnUpdate(Timestep{});
    const TestHelpers::HeadlessImGui imgui([&] { panel.Draw(); });
    imgui.Frame();
    ClickCard(imgui, 0);
    REQUIRE_FALSE(panel.GetSelected().empty());

    context.ProjectDirectory = CreateTestProject(folder, "Second");
    panel.OnUpdate(Timestep{});

    CHECK(panel.GetSelected().empty());
}

TEST_CASE("The content browser draws its window without a project, with an empty folder and with entries")
{
    TestHelpers::UseEditorLocale();
    const TestHelpers::TemporaryFolder folder;
    const std::filesystem::path empty = CreateTestProject(folder, "Empty");
    const std::filesystem::path filled = CreateTestProject(folder, "Filled");
    REQUIRE(Quasar::CreateDirectories(GetAssetsDirectory(filled) / "Textures"));
    REQUIRE(Quasar::WriteTextFile(GetAssetsDirectory(filled) / "notes.txt", "notes"));
    EditorContext context;
    ContentBrowserPanel panel(context);
    const TestHelpers::HeadlessImGui imgui([&] { panel.Draw(); });
    CHECK(ImGui::FindWindowByName(WindowName) != nullptr);

    for (const std::filesystem::path& project : {empty, filled})
    {
        context.ProjectDirectory = project;
        panel.OnUpdate(Timestep{});
        imgui.Frame();

        CHECK(ImGui::FindWindowByName(WindowName) != nullptr);
    }
}

TEST_CASE("The hierarchy of the content browser lists everything in the Assets folder, each folder before its content")
{
    const TestHelpers::TemporaryFolder folder;
    const std::filesystem::path project = CreateTestProject(folder, "Game");
    const std::filesystem::path assets = GetAssetsDirectory(project);
    REQUIRE(Quasar::WriteTextFile(assets / "Textures" / "notes.txt", "notes"));
    REQUIRE(Quasar::CreateDirectories(assets / "Textures" / "Characters"));
    REQUIRE(Quasar::CreateDirectories(assets / "Audio"));
    REQUIRE(Quasar::WriteTextFile(assets / "readme.txt", "readme"));
    EditorContext context{project};
    ContentBrowserPanel panel(context);

    panel.OnUpdate(Timestep{});

    const std::vector<ContentBrowserTreeNode>& tree = panel.GetTree();
    REQUIRE(tree.size() == 6);
    CHECK(tree[0].Path == assets);
    CHECK(tree[1].Path == assets / "Audio");
    CHECK(tree[2].Path == assets / "Textures");
    CHECK(tree[3].Path == assets / "Textures" / "Characters");
    CHECK(tree[4].Path == assets / "Textures" / "notes.txt");
    CHECK(tree[5].Path == assets / "readme.txt");

    const std::vector<int> depths{tree[0].Depth, tree[1].Depth, tree[2].Depth, tree[3].Depth, tree[4].Depth, tree[5].Depth};
    CHECK(depths == std::vector<int>{0, 1, 1, 2, 2, 1});
    const std::vector<bool> directories{tree[0].IsDirectory, tree[1].IsDirectory, tree[2].IsDirectory,
                                        tree[3].IsDirectory, tree[4].IsDirectory, tree[5].IsDirectory};
    CHECK(directories == std::vector<bool>{true, true, true, true, false, false});
    const std::vector<bool> children{tree[0].HasChildren, tree[1].HasChildren, tree[2].HasChildren,
                                     tree[3].HasChildren, tree[4].HasChildren, tree[5].HasChildren};
    CHECK(children == std::vector<bool>{true, false, true, false, false, false});
}

TEST_CASE("The content browser opens a folder of the Assets folder but not a file or a folder outside of it")
{
    const TestHelpers::TemporaryFolder folder;
    const std::filesystem::path project = CreateTestProject(folder, "Game");
    const std::filesystem::path assets = GetAssetsDirectory(project);
    REQUIRE(Quasar::CreateDirectories(assets / "Textures"));
    REQUIRE(Quasar::WriteTextFile(assets / "Textures" / "wall.png", "png"));
    EditorContext context{project};
    ContentBrowserPanel panel(context);
    panel.OnUpdate(Timestep{});

    CHECK(panel.OpenDirectory(assets / "Textures"));
    CHECK(panel.GetShownDirectory() == assets / "Textures");
    CHECK(GetEntryNames(panel) == std::vector<std::filesystem::path>{"wall.png"});

    CHECK_FALSE(panel.OpenDirectory(assets / "Textures" / "wall.png"));
    CHECK_FALSE(panel.OpenDirectory(project));
    CHECK_FALSE(panel.OpenDirectory(assets / "Missing"));
    CHECK(panel.GetShownDirectory() == assets / "Textures");
}

TEST_CASE("A folder of the content browser is opened by double clicking its card but not by clicking it once")
{
    TestHelpers::UseEditorLocale();
    const TestHelpers::TemporaryFolder folder;
    const std::filesystem::path project = CreateTestProject(folder, "Game");
    const std::filesystem::path assets = GetAssetsDirectory(project);
    REQUIRE(Quasar::CreateDirectories(assets / "Textures"));
    EditorContext context{project};
    ContentBrowserPanel panel(context);
    panel.OnUpdate(Timestep{});
    const TestHelpers::HeadlessImGui imgui([&] { panel.Draw(); });
    imgui.Frame();

    ClickCard(imgui, 0);
    CHECK(panel.GetShownDirectory() == assets);

    ClickCard(imgui, 0, 2);
    CHECK(panel.GetShownDirectory() == assets / "Textures");
}

TEST_CASE("The parent folder button of the content browser goes up until the Assets folder")
{
    TestHelpers::UseEditorLocale();
    const TestHelpers::TemporaryFolder folder;
    const std::filesystem::path project = CreateTestProject(folder, "Game");
    const std::filesystem::path assets = GetAssetsDirectory(project);
    REQUIRE(Quasar::CreateDirectories(assets / "Textures" / "Characters"));
    EditorContext context{project};
    ContentBrowserPanel panel(context);
    panel.OnUpdate(Timestep{});
    REQUIRE(panel.OpenDirectory(assets / "Textures" / "Characters"));
    const TestHelpers::HeadlessImGui imgui([&] { panel.Draw(); });
    imgui.Frame();

    ClickParentFolder(imgui);
    CHECK(panel.GetShownDirectory() == assets / "Textures");

    ClickParentFolder(imgui);
    CHECK(panel.GetShownDirectory() == assets);

    ClickParentFolder(imgui);
    CHECK(panel.GetShownDirectory() == assets);
}

TEST_CASE("Clicking a folder in the hierarchy of the content browser opens it")
{
    TestHelpers::UseEditorLocale();
    const TestHelpers::TemporaryFolder folder;
    const std::filesystem::path project = CreateTestProject(folder, "Game");
    const std::filesystem::path assets = GetAssetsDirectory(project);
    REQUIRE(Quasar::CreateDirectories(assets / "Audio"));
    REQUIRE(Quasar::CreateDirectories(assets / "Textures"));
    EditorContext context{project};
    ContentBrowserPanel panel(context);
    panel.OnUpdate(Timestep{});
    const TestHelpers::HeadlessImGui imgui([&] { panel.Draw(); });
    imgui.Frame();

    ClickTreeRow(imgui, 2);
    CHECK(panel.GetShownDirectory() == assets / "Textures");

    ClickTreeRow(imgui, 0);
    CHECK(panel.GetShownDirectory() == assets);
}

TEST_CASE("Clicking a file in the hierarchy of the content browser shows its folder and selects the file")
{
    TestHelpers::UseEditorLocale();
    const TestHelpers::TemporaryFolder folder;
    const std::filesystem::path project = CreateTestProject(folder, "Game");
    const std::filesystem::path assets = GetAssetsDirectory(project);
    REQUIRE(Quasar::CreateDirectories(assets / "Textures"));
    REQUIRE(Quasar::WriteTextFile(assets / "Textures" / "wall.png", "png"));
    EditorContext context{project};
    ContentBrowserPanel panel(context);
    panel.OnUpdate(Timestep{});
    const TestHelpers::HeadlessImGui imgui([&] { panel.Draw(); });
    imgui.Frame();
    ClickTreeRow(imgui, 1);
    REQUIRE(panel.GetShownDirectory() == assets / "Textures");

    ClickTreeRow(imgui, 2);

    CHECK(panel.GetShownDirectory() == assets / "Textures");
    CHECK(panel.GetSelected() == assets / "Textures" / "wall.png");
}
