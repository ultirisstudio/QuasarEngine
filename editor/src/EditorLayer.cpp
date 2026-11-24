#include "EditorLayer.h"

#include "panels/ConsolePanel.h"
#include "panels/GamepadPanel.h"

#include <quasar/core/Application.h>
#include <quasar/events/Event.h>
#include <quasar/io/Path.h>
#include <quasar/log/Log.h>

#include <imgui.h>

#include <optional>

namespace Quasar::Editor
{
    EditorLayer::EditorLayer() : Layer("Editor") {}

    void EditorLayer::OnAttach()
    {
        QUASAR_INFO("Editor started");
        AddPanel<ConsolePanel>();
        AddPanel<GamepadPanel>(Application::Get().GetGamepads());
    }

    void EditorLayer::OnDetach()
    {
        m_Panels.clear();
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
        if (const std::optional<std::filesystem::path> project = m_ProjectPopups.Draw())
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

    void EditorLayer::SetProject(const std::filesystem::path& directory)
    {
        m_Context.ProjectDirectory = directory;
        QUASAR_INFO("Project opened: {}", PathToUtf8(directory));
    }
}
