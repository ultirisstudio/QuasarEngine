#include "helpers/TemporaryFiles.h"

#include <Project.h>
#include <ProjectPopups.h>

#include <quasar/io/File.h>

#include <doctest/doctest.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <filesystem>
#include <optional>

using Quasar::Editor::OpenProject;
using Quasar::Editor::ProjectPopups;

namespace
{
    class HeadlessImGui
    {
    public:
        explicit HeadlessImGui(ProjectPopups& popups) : m_Popups(&popups), m_Previous(ImGui::GetCurrentContext()), m_Context(ImGui::CreateContext())
        {
            ImGui::SetCurrentContext(m_Context);
            ImGuiIO& io = ImGui::GetIO();
            io.DisplaySize = ImVec2(1600.0f, 900.0f);
            io.DeltaTime = 1.0f / 60.0f;
            io.IniFilename = nullptr;
            io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
            Frame();
        }

        ~HeadlessImGui()
        {
            ImGui::DestroyContext(m_Context);
            ImGui::SetCurrentContext(m_Previous);
        }

        HeadlessImGui(const HeadlessImGui&) = delete;
        HeadlessImGui& operator=(const HeadlessImGui&) = delete;
        HeadlessImGui(HeadlessImGui&&) = delete;
        HeadlessImGui& operator=(HeadlessImGui&&) = delete;

        std::optional<std::filesystem::path> Frame() const
        {
            ImGui::NewFrame();
            std::optional<std::filesystem::path> project = m_Popups->Draw();
            ImGui::Render();

            for (ImTextureData* texture : ImGui::GetPlatformIO().Textures)
            {
                if (texture->Status == ImTextureStatus_WantCreate || texture->Status == ImTextureStatus_WantUpdates)
                {
                    texture->SetTexID(1);
                    texture->SetStatus(ImTextureStatus_OK);
                }
                else if (texture->Status == ImTextureStatus_WantDestroy)
                {
                    texture->SetTexID(ImTextureID_Invalid);
                    texture->SetStatus(ImTextureStatus_Destroyed);
                }
            }
            return project;
        }

        void OpenNewProject() const
        {
            m_Popups->RequestNewProject();
            Frame();
            Frame();
        }

        void Type(const char* text) const
        {
            ImGui::GetIO().AddInputCharactersUTF8(text);
            Frame();
        }

        std::optional<std::filesystem::path> Press(ImGuiKey key) const
        {
            ImGui::GetIO().AddKeyEvent(key, true);
            std::optional<std::filesystem::path> project = Frame();
            ImGui::GetIO().AddKeyEvent(key, false);
            if (const std::optional<std::filesystem::path> next = Frame())
            {
                project = next;
            }
            return project;
        }

        static bool IsPopupOpen() { return ImGui::GetTopMostPopupModal() != nullptr; }

    private:
        ProjectPopups* m_Popups;
        ImGuiContext* m_Previous;
        ImGuiContext* m_Context;
    };
}

TEST_SUITE("ProjectPopups")
{
    TEST_CASE("No popup is open before a project is requested")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_closed");
        ProjectPopups popups(folder.GetPath());
        const HeadlessImGui imgui(popups);

        CHECK_FALSE(imgui.Frame().has_value());
        CHECK_FALSE(HeadlessImGui::IsPopupOpen());
    }

    TEST_CASE("Pressing Enter in the New Project popup creates the project in the given location")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_create");
        ProjectPopups popups(folder.GetPath());
        const HeadlessImGui imgui(popups);

        imgui.OpenNewProject();
        REQUIRE(HeadlessImGui::IsPopupOpen());
        imgui.Type("MyGame");
        const std::optional<std::filesystem::path> project = imgui.Press(ImGuiKey_Enter);

        REQUIRE(project.has_value());
        CHECK(*project == (folder.GetPath() / "MyGame").lexically_normal());
        CHECK(OpenProject(*project).IsValid());
        CHECK_FALSE(HeadlessImGui::IsPopupOpen());
    }

    TEST_CASE("The New Project popup creates nothing without a name")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_no_name");
        ProjectPopups popups(folder.GetPath());
        const HeadlessImGui imgui(popups);

        imgui.OpenNewProject();

        CHECK_FALSE(imgui.Press(ImGuiKey_Enter).has_value());
        CHECK(HeadlessImGui::IsPopupOpen());
        CHECK(std::filesystem::is_empty(folder.GetPath()));
    }

    TEST_CASE("A project that cannot be created keeps the New Project popup open")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_existing");
        REQUIRE(Quasar::WriteTextFile(folder.GetPath() / "MyGame" / "notes.txt", "content"));
        ProjectPopups popups(folder.GetPath());
        const HeadlessImGui imgui(popups);

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
        ProjectPopups popups(folder.GetPath());
        const HeadlessImGui imgui(popups);

        imgui.OpenNewProject();
        imgui.Type("MyGame");

        CHECK_FALSE(imgui.Press(ImGuiKey_Escape).has_value());
        CHECK_FALSE(HeadlessImGui::IsPopupOpen());
        CHECK(std::filesystem::is_empty(folder.GetPath()));
    }

    TEST_CASE("A new request clears the name typed in the previous New Project popup")
    {
        const TestHelpers::TemporaryFolder folder("quasar_popups_reopen");
        ProjectPopups popups(folder.GetPath());
        const HeadlessImGui imgui(popups);

        imgui.OpenNewProject();
        imgui.Type("MyGame");
        imgui.Press(ImGuiKey_Escape);
        imgui.OpenNewProject();

        CHECK_FALSE(imgui.Press(ImGuiKey_Enter).has_value());
        CHECK(std::filesystem::is_empty(folder.GetPath()));
    }
}
