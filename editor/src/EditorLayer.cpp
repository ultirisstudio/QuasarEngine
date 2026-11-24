#include "EditorLayer.h"

#include "panels/ConsolePanel.h"
#include "panels/GamepadPanel.h"

#include <quasar/core/Application.h>
#include <quasar/events/Event.h>
#include <quasar/io/Path.h>
#include <quasar/log/Log.h>

#include <imgui.h>

#include <optional>
#include <utility>
#include <vector>

namespace Quasar::Editor
{
    EditorLayer::EditorLayer(std::filesystem::path settingsFile) : Layer("Editor"), m_Settings(std::move(settingsFile)), m_ProjectPopups(m_Settings) {}

    void EditorLayer::OnAttach()
    {
        QUASAR_INFO("Editor started");
        m_ProjectPopups.SetParentWindow(&Application::Get().GetWindow());
        AddPanel<ConsolePanel>();
        AddPanel<GamepadPanel>(Application::Get().GetGamepads());
    }

    void EditorLayer::OnDetach()
    {
        m_Panels.clear();
        m_ProjectPopups.SetParentWindow(nullptr);
        QUASAR_INFO("Editor stopped");
    }

    void EditorLayer::OnUpdate(Timestep timestep)
    {
        for (const Scope<EditorPanel>& panel : m_Panels)
        {
            panel->OnUpdate(timestep);
        }
    }

    void EditorLayer::OnRender()
    {
        for (const Scope<EditorPanel>& panel : m_Panels)
        {
            panel->OnRender();
        }
    }

    void EditorLayer::OnImGuiRender()
    {
        DrawMenuBar();
        if (const std::optional<ProjectResult> project = m_ProjectPopups.Draw())
        {
            SetProject(*project);
        }
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

        for (const Scope<EditorPanel>& panel : m_Panels)
        {
            panel->Draw();
        }
    }

    void EditorLayer::OnEvent(Event& event)
    {
        for (const Scope<EditorPanel>& panel : m_Panels)
        {
            if (event.IsHandled())
            {
                return;
            }
            panel->OnEvent(event);
        }
    }

    void EditorLayer::DrawMenuBar()
    {
        if (!ImGui::BeginMainMenuBar())
        {
            return;
        }

        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Project..."))
            {
                m_ProjectPopups.RequestNewProject();
            }
            if (ImGui::MenuItem("Open Project..."))
            {
                m_ProjectPopups.RequestOpenProject();
            }
            DrawRecentProjectsMenu();
            ImGui::Separator();
            if (ImGui::MenuItem("Quit"))
            {
                Application::Get().Close();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            for (const Scope<EditorPanel>& panel : m_Panels)
            {
                bool open = panel->IsOpen();
                if (ImGui::MenuItem(panel->GetName().c_str(), nullptr, &open))
                {
                    panel->SetOpen(open);
                }
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    void EditorLayer::DrawRecentProjectsMenu()
    {
        const std::vector<std::filesystem::path>& projects = m_Settings.GetRecentProjects();
        if (!ImGui::BeginMenu("Open Recent", !projects.empty()))
        {
            return;
        }
        for (const std::filesystem::path& project : projects)
        {
            if (ImGui::MenuItem(PathToUtf8(project).c_str()))
            {
                m_ProjectPopups.RequestOpenProject(project);
            }
        }
        ImGui::EndMenu();
    }

    void EditorLayer::SetProject(const ProjectResult& project)
    {
        m_Context.ProjectDirectory = project.Directory;
        QUASAR_INFO("Project opened: {} ({})", project.Name, PathToUtf8(project.Directory));
    }
}
