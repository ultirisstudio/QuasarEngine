#include "EditorLayer.h"

#include "EditorText.h"

#include "panels/ConsolePanel.h"
#include "panels/GamepadPanel.h"

#include <quasar/core/Application.h>
#include <quasar/events/Event.h>
#include <quasar/io/Path.h>
#include <quasar/log/Log.h>

#include <imgui.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace Quasar::Editor
{
    EditorLayer::EditorLayer(std::filesystem::path settingsFile) : Layer("Editor"), m_Settings(std::move(settingsFile)), m_ProjectPopups(m_Settings)
    {
        LoadLocales(m_Settings.GetLanguage());
    }

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

        if (ImGui::BeginMenu(TrLabel("menu.file").c_str()))
        {
            if (ImGui::MenuItem(TrLabel("menu.new_project").c_str()))
            {
                m_ProjectPopups.RequestNewProject();
            }
            if (ImGui::MenuItem(TrLabel("menu.open_project").c_str()))
            {
                m_ProjectPopups.RequestOpenProject();
            }
            DrawRecentProjectsMenu();
            ImGui::Separator();
            if (ImGui::MenuItem(TrLabel("menu.quit").c_str()))
            {
                Application::Get().Close();
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(TrLabel("menu.view").c_str()))
        {
            for (const Scope<EditorPanel>& panel : m_Panels)
            {
                bool open = panel->IsOpen();
                if (ImGui::MenuItem(panel->GetLabel().c_str(), nullptr, &open))
                {
                    panel->SetOpen(open);
                }
            }
            ImGui::Separator();
            DrawLanguageMenu();
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    void EditorLayer::DrawRecentProjectsMenu()
    {
        const std::vector<std::filesystem::path>& projects = m_Settings.GetRecentProjects();
        if (!ImGui::BeginMenu(TrLabel("menu.open_recent").c_str(), !projects.empty()))
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

    void EditorLayer::DrawLanguageMenu()
    {
        Localization& localization = GetLocalization();
        if (!ImGui::BeginMenu(TrLabel("menu.language").c_str(), !localization.GetLocales().empty()))
        {
            return;
        }
        for (const std::string& locale : localization.GetLocales())
        {
            const std::string label = std::string(localization.GetLanguageName(locale)) + "###" + locale;
            if (ImGui::MenuItem(label.c_str(), nullptr, locale == localization.GetLocale()) && localization.SetLocale(locale))
            {
                m_Settings.SetLanguage(locale);
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
