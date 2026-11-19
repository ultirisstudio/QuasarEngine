#include "EditorLayer.h"

#include "Project.h"
#include "panels/ConsolePanel.h"

#include <quasar/core/Application.h>
#include <quasar/events/Event.h>
#include <quasar/log/Log.h>

#include <imgui.h>

#include <algorithm>
#include <string_view>
#include <system_error>

namespace Quasar::Editor
{
    namespace
    {
        constexpr const char* NewProjectPopupName = "New Project";
        constexpr const char* OpenProjectPopupName = "Open Project";
        constexpr float FieldWidth = 420.0f;
        constexpr ImVec4 ErrorTextColor{0.898f, 0.325f, 0.294f, 1.0f};

        template <std::size_t Size>
        void CopyToBuffer(std::array<char, Size>& buffer, std::string_view text)
        {
            buffer.fill('\0');
            const std::size_t count = std::min(text.size(), Size - 1);
            std::copy_n(text.begin(), count, buffer.begin());
        }

        void CenterNextPopup()
        {
            ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        }
    }

    EditorLayer::EditorLayer() : Layer("Editor") {}

    void EditorLayer::OnAttach()
    {
        QUASAR_INFO("Editor started");
        AddPanel<ConsolePanel>();
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
        DrawProjectPopups();
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
                RequestProjectPopup(ProjectPopup::NewProject);
            }
            if (ImGui::MenuItem("Open Project..."))
            {
                RequestProjectPopup(ProjectPopup::OpenProject);
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

    void EditorLayer::RequestProjectPopup(ProjectPopup popup)
    {
        m_RequestedPopup = popup;
        m_ProjectName.fill('\0');
        m_ProjectError.clear();

        std::error_code error;
        const std::filesystem::path workingDirectory = std::filesystem::current_path(error);
        CopyToBuffer(m_ProjectLocation, error ? std::string() : PathToUtf8(workingDirectory));
    }

    void EditorLayer::DrawProjectPopups()
    {
        switch (m_RequestedPopup)
        {
            case ProjectPopup::NewProject:
                ImGui::OpenPopup(NewProjectPopupName);
                break;
            case ProjectPopup::OpenProject:
                ImGui::OpenPopup(OpenProjectPopupName);
                break;
            case ProjectPopup::None:
                break;
        }
        m_RequestedPopup = ProjectPopup::None;

        DrawNewProjectPopup();
        DrawOpenProjectPopup();
    }

    void EditorLayer::DrawNewProjectPopup()
    {
        CenterNextPopup();
        if (!ImGui::BeginPopupModal(NewProjectPopupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            return;
        }

        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(FieldWidth);
        ImGui::InputText("Name", m_ProjectName.data(), m_ProjectName.size());
        ImGui::SetNextItemWidth(FieldWidth);
        ImGui::InputText("Location", m_ProjectLocation.data(), m_ProjectLocation.size());
        DrawProjectError();

        if (ImGui::Button("Create"))
        {
            ApplyProjectResult(CreateProject(PathFromUtf8(m_ProjectLocation.data()), m_ProjectName.data()));
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    void EditorLayer::DrawOpenProjectPopup()
    {
        CenterNextPopup();
        if (!ImGui::BeginPopupModal(OpenProjectPopupName, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            return;
        }

        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::SetNextItemWidth(FieldWidth);
        ImGui::InputText("Folder", m_ProjectLocation.data(), m_ProjectLocation.size());
        DrawProjectError();

        if (ImGui::Button("Open"))
        {
            ApplyProjectResult(OpenProject(PathFromUtf8(m_ProjectLocation.data())));
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    void EditorLayer::DrawProjectError() const
    {
        if (m_ProjectError.empty())
        {
            return;
        }

        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + FieldWidth);
        ImGui::TextColored(ErrorTextColor, "%s", m_ProjectError.c_str());
        ImGui::PopTextWrapPos();
    }

    void EditorLayer::ApplyProjectResult(const ProjectResult& result)
    {
        if(!result.IsValid())
        {
            m_ProjectError = result.Error;
            return;
        }

        SetProject(result.Directory);
        ImGui::CloseCurrentPopup();
    }

    void EditorLayer::SetProject(const std::filesystem::path& directory)
    {
        m_Context.ProjectDirectory = directory;
        QUASAR_INFO("Project opened: {}", PathToUtf8(directory));
    }
}
